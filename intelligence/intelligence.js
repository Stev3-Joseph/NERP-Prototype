
const fs = require("fs");
const path = require("path");

const filePath = path.join(
    __dirname,
    "..",
    "datastore",
    "intelligence.json"
);

function loadReports() {
    return JSON.parse(fs.readFileSync(filePath, "utf8"));
}

// View all reports
function viewReports() {
    const reports = loadReports();

    reports.forEach(report => {
        console.log("\n-------------------------");
        console.log("Report ID:", report.reportId);
        console.log("Source:", report.reportSource);
        console.log("Threat Level:", report.threatLevel);
        console.log("Region:", report.region);
        console.log("Classification:", report.classificationHint);
        console.log("Summary:", report.summary);
    });
}

// Search by report ID
function searchReport(id) {
    const reports = loadReports();

    const report = reports.find(r => r.reportId === id);

    if (!report) {
        console.log("Report not found.");
        return;
    }

    console.log("\nReport found:");
    console.log(report);
}

// Filter by threat level
function filterByThreat(level) {
    const reports = loadReports();

    const filtered = reports.filter(
        r => r.threatLevel.toLowerCase() === level.toLowerCase()
    );

    if (filtered.length === 0) {
        console.log("No reports found for this threat level.");
        return;
    }

    filtered.forEach(report => {
        console.log(
            `${report.reportId} | ${report.threatLevel} | ` +
            `${report.region} | ${report.classificationHint}`
        );
    });
}

module.exports = {
    viewReports,
    searchReport,
    filterByThreat
};
