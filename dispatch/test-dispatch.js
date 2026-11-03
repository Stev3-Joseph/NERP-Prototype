
const readline = require("readline/promises");
const { stdin: input, stdout: output } = require("process");

const {
    viewDispatch,
    assignResponder,
    updateStatus
} = require("./dispatch");

async function main() {
    const rl = readline.createInterface({ input, output });

    console.log("\n===== NERP DISPATCH =====");
    console.log("1. View dispatch records");
    console.log("2. Assign responder");
    console.log("3. Update dispatch status");

    const choice = await rl.question("\nSelect option: ");

    if (choice === "1") {
        viewDispatch();

    } else if (choice === "2") {
        const incidentId = await rl.question("Incident ID: ");
        const responderId = await rl.question("Responder ID: ");

        assignResponder(incidentId.trim(), responderId.trim());

    } else if (choice === "3") {
        const dispatchId = await rl.question("Dispatch ID: ");
        const status = await rl.question(
            "New status (Pending/In Progress/Escalated/Completed): "
        );

        updateStatus(dispatchId.trim(), status.trim());

    } else {
        console.log("Invalid option.");
    }

    rl.close();
}

main();
