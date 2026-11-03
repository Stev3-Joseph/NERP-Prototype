
const fs = require("fs");
const path = require("path");

function loadData(filename) {
    const file = path.join(__dirname, "..", "datastore", filename);
    return JSON.parse(fs.readFileSync(file, "utf8"));
}

// 1. Operational view: full incident records
function operationalView() {
    const incidents = loadData("incidents.json");

    console.log("\n===== OPERATIONAL VIEW =====");
    console.table(incidents);
}

// 2. Analytics view: remove direct caller identifiers
function analyticsView() {
    const incidents = loadData("incidents.json");

    const sanitised = incidents.map(incident => {
        const {
            callerName,
            callerPhone,
            ...remainingData
        } = incident;

        return remainingData;
    });

    console.log("\n===== ANALYTICS VIEW =====");
    console.table(sanitised);
}

// 3. Research view: anonymise identifying information
function researchView() {
    const incidents = loadData("incidents.json");

    const anonymised = incidents.map(incident => ({
        incidentId: incident.incidentId,
        date: incident.timestamp
            ? incident.timestamp.split("T")[0]
            : null,
        incidentType: incident.incidentType,
        casualties: incident.casualties,
        priority: incident.priority,
        assignedAgency: incident.assignedAgency
    }));

    console.log("\n===== RESEARCH VIEW =====");
    console.table(anonymised);
}

// 4. Intelligence classification
function classificationView() {
    const reports = loadData("intelligence.json");

    console.log("\n===== CLASSIFIED INTELLIGENCE =====");

    reports.forEach(report => {
        console.log(
            `${report.reportId} | ${report.classificationHint}`
        );
    });
}

module.exports = {
    operationalView,
    analyticsView,
    researchView,
    classificationView
};
