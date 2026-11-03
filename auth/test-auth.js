
const readline = require("readline");
const { login } = require("./auth");

const rl = readline.createInterface({
    input: process.stdin,
    output: process.stdout
});

rl.question("Enter Responder ID: ", (id) => {
    const user = login(id.trim());

    if (user) {
        console.log(`Welcome to NERP, ${user.name}!`);
    }

    rl.close();
});
