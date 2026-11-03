
const readline = require("readline/promises");
const { stdin: input, stdout: output } = require("process");

const {
    operationalView,
    analyticsView,
    researchView,
    classificationView
} = require("./governance");

async function main() {
    const rl = readline.createInterface({ input, output });

    console.log("\n===== NERP INFORMATION GOVERNANCE =====");
    console.log("1. Operational view");
    console.log("2. Analytics view");
    console.log("3. Research view");
    console.log("4. Intelligence classification");

    const choice = await rl.question("\nSelect option: ");

    switch (choice) {
        case "1":
            operationalView();
            break;
        case "2":
            analyticsView();
            break;
        case "3":
            researchView();
            break;
        case "4":
            classificationView();
            break;
        default:
            console.log("Invalid option.");
    }

    rl.close();
}

main();
