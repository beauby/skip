function onUpdate(msg, context, events) {
    events.emit('counter', 'skip.updates_received', 1);
    for (const update of JSON.parse(msg.data)) {
        const uuid = update[0];
        if (uuid in (context.vars.write_timestamps ?? {})) {
            context.vars.writes_replicated = (context.vars.writes_replicated ?? 0) + 1;
            const timeDelta = Date.now() - context.vars.write_timestamps[uuid];
            events.emit('counter', 'skip.writes_replicated', 1);
            events.emit('histogram', 'skip.replication_time', timeDelta);
        }
    }
}

function writeHook(collection, data, context, events) {
    context.vars.writes_issued = (context.vars.writes_issued ?? 0) + 1;
    events.emit('counter', 'skip.writes_issued', 1);
    context.vars.write_timestamps ??= {};
    context.vars.write_timestamps[data[0][0]] = Date.now();
}

function allWritesReplicated(context, next) {
    next(context.vars.writes_issued != context.vars.writes_replicated);
}

module.exports = {
    onUpdate,
    writeHook,
    allWritesReplicated,
}
