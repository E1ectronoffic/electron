'use strict'

const bindings = process.atomBinding('app')
const {app, App} = bindings

// Only one app object permitted.
module.exports = app

const electron = require('electron')
const {deprecate, Menu} = electron
const {EventEmitter} = require('events')
const fs = require('fs')
const path = require('path')

Object.setPrototypeOf(App.prototype, EventEmitter.prototype)

let appPath = null

Object.assign(app, {
  getAppPath () { return appPath },
  setAppPath (path) { appPath = path },
  setApplicationMenu (menu) {
    return Menu.setApplicationMenu(menu)
  },
  getApplicationMenu () {
    return Menu.getApplicationMenu()
  },
  commandLine: {
    appendSwitch (...args) {
      const castedArgs = args.map((arg) => {
        return typeof arg !== 'string' ? `${arg}` : arg
      })
      return bindings.appendSwitch(...castedArgs)
    },
    appendArgument (...args) {
      const castedArgs = args.map((arg) => {
        return typeof arg !== 'string' ? `${arg}` : arg
      })
      return bindings.appendArgument(...castedArgs)
    }
  }
})

// Wrap methods that default to process.execPath with Squirrel related syntax
const squirrelUpdateExePath = path.join(path.dirname(process.execPath), '..', 'update.exe')

if (process.platform === 'win32' && fs.existsSync(squirrelUpdateExePath)) {
  const wrapProtocolClientMethod = (methodName) => {
    const originalMethod = app[methodName]
    app[methodName] = (protocol, targetPath, args = []) => {
      if (targetPath) {
        return originalMethod(protocol, targetPath, args)
      }
      return originalMethod(protocol, squirrelUpdateExePath, `--processStart="${path.basename(process.execPath)}" --process-start-args=${JSON.stringify(args.join(' '))}`)
    }
  };
  wrapProtocolClientMethod('setAsDefaultProtocolClient')
  wrapProtocolClientMethod('removeAsDefaultProtocolClient')
  wrapProtocolClientMethod('isDefaultProtocolClient')
}

if (process.platform === 'darwin') {
  app.dock = {
    bounce (type = 'informational') {
      return bindings.dockBounce(type)
    },
    cancelBounce: bindings.dockCancelBounce,
    downloadFinished: bindings.dockDownloadFinished,
    setBadge: bindings.dockSetBadgeText,
    getBadge: bindings.dockGetBadgeText,
    hide: bindings.dockHide,
    show: bindings.dockShow,
    isVisible: bindings.dockIsVisible,
    setMenu: bindings.dockSetMenu,
    setIcon: bindings.dockSetIcon
  }
}

if (process.platform === 'linux') {
  app.launcher = {
    setBadgeCount: bindings.unityLauncherSetBadgeCount,
    getBadgeCount: bindings.unityLauncherGetBadgeCount,
    isCounterBadgeAvailable: bindings.unityLauncherAvailable,
    isUnityRunning: bindings.unityLauncherAvailable
  }
}

app.allowNTLMCredentialsForAllDomains = function (allow) {
  if (!process.noDeprecations) {
    deprecate.warn('app.allowNTLMCredentialsForAllDomains', 'session.allowNTLMCredentialsForDomains')
  }
  let domains = allow ? '*' : ''
  if (!this.isReady()) {
    this.commandLine.appendSwitch('auth-server-whitelist', domains)
  } else {
    electron.session.defaultSession.allowNTLMCredentialsForDomains(domains)
  }
}

// Routes the events to webContents.
const events = ['login', 'certificate-error', 'select-client-certificate']
for (let name of events) {
  app.on(name, (event, webContents, ...args) => {
    webContents.emit.apply(webContents, [name, event].concat(args))
  })
}

// Wrappers for native classes.
const {DownloadItem} = process.atomBinding('download_item')
Object.setPrototypeOf(DownloadItem.prototype, EventEmitter.prototype)
