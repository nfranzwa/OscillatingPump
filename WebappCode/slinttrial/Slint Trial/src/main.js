import * as slint from "slint-ui";

const ui = slint.loadFile(new URL("./ui/app-window.slint", import.meta.url));
const mainWindow = new ui.MainWindow();
await mainWindow.run();