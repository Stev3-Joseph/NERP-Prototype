
const fs = require("fs");
const path = require("path");

function loadData(filename) {
    const file = path.join(__dirname, "..", "datastore", filename);
    return JSON.parse(fs.readFileSync(file, "utf8"));
}

function saveData(filename, data) {
    const file = path.join(__dirname, "..", "datastore", filename);
    fs.writeFileSync(file, JSON.stringify(data, null, 2));
}

// View dispatch records
function viewDispatch() {
    const dispatches = loadData("dispatch.json");

    dispatches.forEach(d => {
        console.log(
            `${d.dispatchId} | ${d.incidentId} | ${d.unit} | ${d.status}`
        );
    });
}

// Assign a responder to an incident
function assignResponder(incidentId, responderId) {
    const incidents = loadData("incidents.json");
    const responders = loadData("responders.json");
    const dispatches = loadData("dispatch.json");

    const incident = incidents.find(i => i.incidentId === incidentId);
    const responder = responders.find(r => r.responderId === responderId);

    if (!incident) {
        console.log("Incident not found.");
        return;
    }

    if (!responder) {
        console.log("Responder not found.");
        return;
    }

    if (responder.currentAssignment) {
        console.log("Responder is already assigned.");
        return;
    }

    const numbers = dispatches.map(d =>
        Number(d.dispatchId.replace("D", ""))
    );

    const nextNumber = Math.max(0, ...numbers) + 1;
    const dispatchId = `D${String(nextNumber).padStart(3, "0")}`;

    const newDispatch = {
        dispatchId,
        incidentId,
        unit: responder.role,
        departureTime: new Date().toISOString(),
        arrivalTime: null,
        status: "In Progress"
    };

    dispatches.push(newDispatch);
    responder.currentAssignment = incidentId;

    saveData("dispatch.json", dispatches);
    saveData("responders.json", responders);

    console.log("Responder assigned successfully!");
    console.log("Dispatch ID:", dispatchId);
}

// Update dispatch status
function updateStatus(dispatchId, newStatus) {
    const dispatches = loadData("dispatch.json");
    const responders = loadData("responders.json");

    const dispatch = dispatches.find(d => d.dispatchId === dispatchId);

    if (!dispatch) {
        console.log("Dispatch not found.");
        return;
    }

    const allowedStatuses = [
        "Pending",
        "In Progress",
        "Escalated",
        "Completed"
    ];

    if (!allowedStatuses.includes(newStatus)) {
        console.log("Invalid status.");
        return;
    }

    dispatch.status = newStatus;

    if (newStatus === "Completed") {
        dispatch.arrivalTime = new Date().toISOString();

        const responder = responders.find(
            r => r.currentAssignment === dispatch.incidentId
        );

        if (responder) {
            responder.currentAssignment = null;
            saveData("responders.json", responders);
        }
    }

    saveData("dispatch.json", dispatches);

    console.log("Dispatch status updated:", newStatus);
}

module.exports = {
    viewDispatch,
    assignResponder,
    updateStatus
};
