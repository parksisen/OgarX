const fs = require("fs");
const path = require("path");
const browserify = require("browserify");
const minifier = require("babel-minify");

const MAIN_IN  = path.resolve(__dirname, "webgl", "game", "main.js");
const MAIN_OUT = path.resolve(__dirname, "public", "js", "main.min.js");

const RENDERER_IN  = path.resolve(__dirname, "webgl", "game", "renderer.js");
const RENDERER_OUT = path.resolve(__dirname, "public", "js", "renderer.min.js");

const LOADER_IN  = path.resolve(__dirname, "webgl", "game", "loader.js");
const LOADER_OUT = path.resolve(__dirname, "public", "js", "loader.min.js");

const CONTROL_IN  = path.resolve(__dirname, "webgl", "control", "control.js");
const CONTROL_OUT = path.resolve(__dirname, "public", "js", "control.min.js");

const SW_IN  = path.resolve(__dirname, "src", "worker.js");
const SW_OUT = path.resolve(__dirname, "public", "js", "sw.min.js");

/** @returns {Promise<string>} */
const streamToString = stream => {
    const chunks = [];
    return new Promise((resolve, reject) => {
        stream.on('data', chunk => chunks.push(chunk))
        stream.on('error', reject)
        stream.on('end', () => resolve(Buffer.concat(chunks).toString('utf8')))
    });
}

(async () => {
    let code = await streamToString(browserify(RENDERER_IN).bundle());
    fs.writeFileSync(RENDERER_OUT, minifier(code).code);

    code = await streamToString(browserify(SW_IN).bundle());
    fs.writeFileSync(SW_OUT, minifier(code).code);

    code = await streamToString(browserify(MAIN_IN).bundle());
    fs.writeFileSync(MAIN_OUT, minifier(code).code);

    code = await streamToString(browserify(CONTROL_IN).bundle());
    fs.writeFileSync(CONTROL_OUT, minifier(code).code);

    code = await streamToString(browserify(LOADER_IN).bundle());
    fs.writeFileSync(LOADER_OUT, minifier(code).code);
})();
