
const fs = require("fs");
const path = require("path");

const filePath = path.join(
    __dirname, "..", "datastore", "incidents.json"
);

function loadIncidents() {
    return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

function saveIncidents(incidents) {
    fs.writeFileSync(filePath, JSON.stringify(incidents, null, 2));
}

// View all incidents
function viewIncidents() {
    const incidents = loadIncidents();

    incidents.forEach(incident => {
        console.log(
            `${incident.incidentId} | ${incident.incidentType} | ` +
            `${incident.priority} | ${incident.location}`
        );
    });
}

// Create a new incident
function createIncident(data) {
    const incidents = loadIncidents();

    const numbers = incidents.map(i =>
        Number(i.incidentId.replace("INC-", ""))
    );

    const nextNumber = Math.max(0, ...numbers) + 1;
    const incidentId = `INC-${String(nextNumber).padStart(3, "0")}`;

    const newIncident = {
        incidentId,
        timestamp: new Date().toISOString(),
        callerName: data.callerName,
        callerPhone: data.callerPhone,
        location: data.location,
        incidentType: data.incidentType,
        casualties: Number(data.casualties),
        priority: data.priority,
        assignedAgency: data.assignedAgency
    };

    incidents.push(newIncident);
    saveIncidents(incidents);

    console.log("Incident created:", incidentId);
}

// Update incident priority
function updatePriority(id, newPriority) {
    const incidents = loadIncidents();

    const incident = incidents.find(i => i.incidentId === id);

    if (!incident) {
        console.log("Incident not found.");
        return;
    }

    incident.priority = newPriority;
    saveIncidents(incidents);

    console.log(`${id} priority updated to ${newPriority}`);
}

module.exports = {
    viewIncidents,
    createIncident,
    updatePriority
};
