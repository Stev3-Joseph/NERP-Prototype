
const fs = require("fs");
const path = require("path");

function loadData(filename) {
    const file = path.join(__dirname, "..", "datastore", filename);
    return JSON.parse(fs.readFileSync(file, "utf8"));
}

function generateAnalytics() {
    const incidents = loadData("incidents.json");
    const dispatches = loadData("dispatch.json");

    console.log("\n========== NERP ANALYTICS ==========\n");

    // Total incidents
    console.log("Total Incidents:", incidents.length);

    // Priority distribution
    const priorities = {};

    incidents.forEach(incident => {
        priorities[incident.priority] =
            (priorities[incident.priority] || 0) + 1;
    });

    console.log("\n--- Priority Distribution ---");

    Object.entries(priorities).forEach(([priority, count]) => {
        console.log(`${priority}: ${count}`);
    });

    // Incidents by agency
    const agencies = {};

    incidents.forEach(incident => {
        agencies[incident.assignedAgency] =
            (agencies[incident.assignedAgency] || 0) + 1;
    });

    console.log("\n--- Incidents by Agency ---");

    Object.entries(agencies).forEach(([agency, count]) => {
        console.log(`${agency}: ${count}`);
    });

    // Total casualties
    const casualties = incidents.reduce(
        (total, incident) => total + Number(incident.casualties || 0),
        0
    );

    console.log("\nTotal Casualties:", casualties);

    // Dispatch status
    const statuses = {};

    dispatches.forEach(dispatch => {
        statuses[dispatch.status] =
            (statuses[dispatch.status] || 0) + 1;
    });

    console.log("\n--- Dispatch Status ---");

    Object.entries(statuses).forEach(([status, count]) => {
        console.log(`${status}: ${count}`);
    });

    // Average response time
    const completed = dispatches.filter(
        d => d.status === "Completed" &&
             d.departureTime &&
             d.arrivalTime
    );

    if (completed.length > 0) {
        const totalMinutes = completed.reduce((total, d) => {
            const departure = new Date(d.departureTime);
            const arrival = new Date(d.arrivalTime);

            return total + (arrival - departure) / 60000;
        }, 0);

        console.log(
            "\nAverage Response Time:",
            (totalMinutes / completed.length).toFixed(2),
            "minutes"
        );
    } else {
        console.log("\nAverage Response Time: No completed records");
    }

    console.log("\n====================================");
}

module.exports = { generateAnalytics };
