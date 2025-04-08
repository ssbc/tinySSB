// eventEmitter.js

/**
 * Class representing an EventEmitter.
 * This class allows different parts of an application to communicate with each other
 * by emitting and listening to events.
 */
class EventEmitter {

    /**
    * Create an EventEmitter.
    * Initializes an empty events object to store event listeners.
    */
    constructor() {
        this.events = {};
    }

    /**
     * Register an event listener for a specific event.
     * If the event does not exist, it is created.
     *
     * @param {string} event - The name of the event.
     * @param {function} listener - The callback function to execute when the event is emitted.
     */
    on(event, listener) {
        if (!this.events[event]) {
            this.events[event] = [];
        }
        this.events[event].push(listener);
    }

    /**
     * Emit a specific event, executing all registered listeners for that event.
     * Additional arguments are passed to the listener functions.
     *
     * @param {string} event - The name of the event to emit.
     * @param {...*} args - The arguments to pass to the listener functions.
     * @returns {Array} - The results of the listener functions.
     */
    emit(event, ...args) {
        if (!this.events[event]) return [];

        return this.events[event].map(listener => listener(...args));
    }
}

// Create a global instance of EventEmitter and assign it to window object
window.eventEmitter = new EventEmitter();
