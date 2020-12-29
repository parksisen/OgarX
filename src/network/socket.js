const Handler = require("../game/handle");
const Protocols = require("./protocols");

module.exports = class SocketHandler extends Handler {
    /** 
     * @param {import("../game")} game
     * @param {import("uWebSockets.js").WebSocket} ws 
     */
    constructor(game, ws) {
        super(game);
        this.ws = ws;
        this.protocol = null;
        this.ws.subscribe("broadcast");
    }

    /** @param {import("../game/controller")} controller */
    onSpawn(controller) {
        this.protocol && this.protocol.onSpawn(controller);
    }

    onUpdate() {
        this.protocol && this.protocol.onUpdate();
    }

    /** @param {DataView} view */
    onMessage(view) {
        try {
            if (!this.protocol) {
                const Protocol = Protocols.find(p => p.handshake(view));
                if (!Protocol) this.ws.end(1003, "Ambiguous protocol");
                this.protocol = new Protocol(this);
            } else this.protocol.onMessage(view);
        } catch (e) {
            console.error(e);
        }
    }

    /** 
     * @param {import("../game/controller")} controller
     * @param {string} message
     */
    onChat(controller, message) {
        this.protocol && this.protocol.onChat(controller, message);
    }
}