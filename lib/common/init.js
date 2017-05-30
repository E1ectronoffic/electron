const timers = require('timers')

process.atomBinding = require('./atom-binding-setup')(process.binding, process.type)

// setImmediate and process.nextTick make use of uv_check and uv_prepare to
// run the callbacks, however since we only run uv loop on requests, the
// callbacks won't be called until something else activates the uv loop,
// which would delay the callbacks for an arbitrarily long time. So we should
// initially activate the uv loop once setImmediate and process.nextTick are
// called.
var wrapWithActivateUvLoop = function (func) {
  return function () {
    process.activateUvLoop()
    return func.apply(this, arguments)
  }
}

process.nextTick = wrapWithActivateUvLoop(process.nextTick)

global.setImmediate = wrapWithActivateUvLoop(timers.setImmediate)

global.clearImmediate = timers.clearImmediate

if (process.type === 'browser') {
  // setTimeout needs to update the polling timeout of the event loop, when
  // called under Chromium's event loop node's event loop won't get a chance
  // to update the timeout, so we have to force node's event loop to
  // recalculate the timeout in the browser process.
  global.setTimeout = wrapWithActivateUvLoop(timers.setTimeout)
  global.setInterval = wrapWithActivateUvLoop(timers.setInterval)
}

if (process.platform === 'win32') {
  // Always returns EOF for stdin stream.
  const {Readable} = require('stream')
  const stdin = new Readable()
  stdin.push(null)
  process.__defineGetter__('stdin', function () {
    return stdin
  })

  // If we're running as a Windows Store app, __dirname will be set
  // to C:/Program Files/WindowsApps.
  //
  // Nobody else gets to install there; changing the path is forbidden
  // We can therefore say that we're running as appx
  if (__dirname.includes('\\WindowsApps\\')) {
    process.windowsStore = true
  }
}
