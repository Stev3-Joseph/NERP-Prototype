
const fs = require("fs");
const path = require("path");

const respondersPath = path.join(
    __dirname,
    "..",
    "datastore",
    "responders.json"
);

function getResponders() {
    return JSON.parse(fs.readFileSync(respondersPath, "utf8"));
}

function login(responderId) {
    const responders = getResponders();

    const user = responders.find(
        responder => responder.responderId === responderId
    );

    if (!user) {
        console.log("Login failed: Invalid responder ID.");
        return null;
    }

    console.log("\nLogin successful!");
    console.log("-------------------------");
    console.log("Name:", user.name);
    console.log("Role:", user.role);
    console.log("Clearance:", user.clearanceLevel);
    console.log("-------------------------");

    return user;
}

module.exports = { login };
