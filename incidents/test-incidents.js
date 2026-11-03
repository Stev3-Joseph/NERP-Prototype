
const readline = require("readline");

const {
    viewIncidents,
    createIncident,
    updatePriority
} = require("./incidents");

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

console.log("\n===== INCIDENT MANAGEMENT =====");
console.log("1. View incidents");
console.log("2. Create incident");
console.log("3. Update priority");

rl.question("\nSelect option: ", (choice) => {

    if (choice === "1") {
        viewIncidents();
        rl.close();

    } else if (choice === "2") {
        rl.question("Caller name: ", callerName => {
            rl.question("Caller phone: ", callerPhone => {
                rl.question("Location: ", location => {
                    rl.question("Incident type: ", incidentType => {
                        rl.question("Casualties: ", casualties => {
                            rl.question("Priority (Critical/High/Medium): ", priority => {
                                rl.question("Assigned agency: ", assignedAgency => {

                                    createIncident({
                                        callerName,
                                        callerPhone,
                                        location,
                                        incidentType,
                                        casualties,
                                        priority,
                                        assignedAgency
                                    });

                                    rl.close();
                                });
                            });
                        });
                    });
                });
            });
        });

    } else if (choice === "3") {
        rl.question("Incident ID: ", id => {
            rl.question("New priority: ", priority => {
                updatePriority(id, priority);
                rl.close();
            });
        });

    } else {
        console.log("Invalid option.");
        rl.close();
    }
});
