
const readline = require("readline/promises");
const { stdin: input, stdout: output } = require("process");

const {
    viewReports,
    searchReport,
    filterByThreat
} = require("./intelligence");

async function main() {
    const rl = readline.createInterface({ input, output });

    console.log("\n===== NERP INTELLIGENCE =====");
    console.log("1. View all reports");
    console.log("2. Search report");
    console.log("3. Filter by threat level");

    const choice = await rl.question("\nSelect option: ");

    if (choice === "1") {
        viewReports();

    } else if (choice === "2") {
        const id = await rl.question("Enter report ID: ");
        searchReport(id.trim());

    } else if (choice === "3") {
        const level = await rl.question("Enter threat level: ");
        filterByThreat(level.trim());

    } else {
        console.log("Invalid option.");
    }

    rl.close();
}

main();
