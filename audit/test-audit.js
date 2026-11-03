
const readline = require("readline/promises");
const { stdin: input, stdout: output } = require("process");

const { logEvent, viewAudit } = require("./audit");

async function main() {
    const rl = readline.createInterface({ input, output });

    console.log("\n===== NERP AUDIT SYSTEM =====");
    console.log("1. View audit history");
    console.log("2. Record new event");

    const choice = await rl.question("\nSelect option: ");

    if (choice === "1") {
        viewAudit();

    } else if (choice === "2") {
        const actor = await rl.question("Actor: ");
        const action = await rl.question("Action: ");
        const resource = await rl.question("Resource: ");

        logEvent(actor, action, resource);

    } else {
        console.log("Invalid option.");
    }

    rl.close();
}

main();
