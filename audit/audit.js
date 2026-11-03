
const fs = require("fs");
const path = require("path");

const filePath = path.join(
    __dirname,
    "..",
    "datastore",
    "audit.json"
);

function loadAudit() {
    return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function saveAudit(records) {
    fs.writeFileSync(filePath, JSON.stringify(records, null, 2));
}

// Record an event
function logEvent(actor, action, resource) {
    const records = loadAudit();

    const numbers = records.map(r =>
        Number(r.eventId.replace("AUD-", ""))
    );

    const nextNumber = Math.max(0, ...numbers) + 1;

    const newEvent = {
        eventId: `AUD-${String(nextNumber).padStart(3, "0")}`,
        actor,
        action,
        resource,
        timestamp: new Date().toISOString()
    };

    records.push(newEvent);
    saveAudit(records);

    console.log("Audit event recorded:", newEvent.eventId);
}

// View audit history
function viewAudit() {
    const records = loadAudit();

    records.forEach(event => {
        console.log("\n-------------------------");
        console.log("Event ID:", event.eventId);
        console.log("Actor:", event.actor);
        console.log("Action:", event.action);
        console.log("Resource:", event.resource);
        console.log("Timestamp:", event.timestamp);
    });
}

module.exports = {
    logEvent,
    viewAudit
};
