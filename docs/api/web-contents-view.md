# WebContentsView

> A View that displays a WebContents.

Process: [Main](../glossary.md#main-process)

This module cannot be used until the `ready` event of the `app`
module is emitted.

```js
const { BaseWindow, WebContentsView } = require('electron')
const win = new BaseWindow({ width: 800, height: 400 })

const view1 = new WebContentsView()
win.contentView.addChildView(view1)
view1.webContents.loadURL('https://electronjs.org')
view1.setBounds({ x: 0, y: 0, width: 400, height: 400 })

const view2 = new WebContentsView()
win.contentView.addChildView(view2)
view2.webContents.loadURL('https://github.com/electron/electron')
view2.setBounds({ x: 400, y: 0, width: 400, height: 400 })
```

## Class: WebContentsView extends `View`

> A View that displays a WebContents.

Process: [Main](../glossary.md#main-process)

`WebContentsView` is an [EventEmitter][event-emitter].

### `new WebContentsView([options])`

* `options` WebPreferences (optional) - See the [`webPreferences` option in
  BrowserWindow](./browser-window.md#new-browserwindowoptions).

Creates an empty WebContentsView.

### Instance Events

Objects created with `new View` emit the following events:

#### Event: 'bounds-changed'

Emitted when the view's bounds have changed in response to being laid out. The
new bounds can be retrieved with [`view.getBounds()`](#viewgetbounds).

### Instance Methods

Objects created with `new WebContentsView` have the following instance methods:

#### `view.addChildView(view[, index])`

* `view` View - Child view to add.
* `index` Integer (optional) - Index at which to insert the child view.
  Defaults to adding the child at the end of the child list.

#### `view.removeChildView(view)`

* `view` View - Child view to remove.

#### `view.setBounds(bounds)`

* `bounds` [Rectangle](structures/rectangle.md) - New bounds of the View.

#### `view.getBounds()`

Returns [`Rectangle`](structures/rectangle.md) - The bounds of this View, relative to its parent.

#### `view.setBackgroundColor(color)`

* `color` string - Color in Hex, RGB, ARGB, HSL, HSLA or named CSS color format. The alpha channel is
  optional for the hex type.

Examples of valid `color` values:

* Hex
  * `#fff` (RGB)
  * `#ffff` (ARGB)
  * `#ffffff` (RRGGBB)
  * `#ffffffff` (AARRGGBB)
* RGB
  * `rgb\(([\d]+),\s*([\d]+),\s*([\d]+)\)`
    * e.g. `rgb(255, 255, 255)`
* RGBA
  * `rgba\(([\d]+),\s*([\d]+),\s*([\d]+),\s*([\d.]+)\)`
    * e.g. `rgba(255, 255, 255, 1.0)`
* HSL
  * `hsl\((-?[\d.]+),\s*([\d.]+)%,\s*([\d.]+)%\)`
    * e.g. `hsl(200, 20%, 50%)`
* HSLA
  * `hsla\((-?[\d.]+),\s*([\d.]+)%,\s*([\d.]+)%,\s*([\d.]+)\)`
    * e.g. `hsla(200, 20%, 50%, 0.5)`
* Color name
  * Options are listed in [SkParseColor.cpp](https://source.chromium.org/chromium/chromium/src/+/main:third_party/skia/src/utils/SkParseColor.cpp;l=11-152;drc=eea4bf52cb0d55e2a39c828b017c80a5ee054148)
  * Similar to CSS Color Module Level 3 keywords, but case-sensitive.
    * e.g. `blueviolet` or `red`

**Note:** Hex format with alpha takes `AARRGGBB` or `ARGB`, _not_ `RRGGBBAA` or `RGB`.

#### `view.setVisible(visible)`

* `visible` boolean - If false, the view will be hidden from display.

### Instance Properties

Objects created with `new WebContentsView` have the following properties:

#### `view.webContents` _Readonly_

A `WebContents` property containing a reference to the displayed `WebContents`.
Use this to interact with the `WebContents`, for instance to load a URL.

```js
const { WebContentsView } = require('electron')
const view = new WebContentsView()
view.webContents.loadURL('https://electronjs.org/')
```

#### `view.children` _Readonly_

A `View[]` property representing the child views of this view.

[event-emitter]: https://nodejs.org/api/events.html#events_class_eventemitter
