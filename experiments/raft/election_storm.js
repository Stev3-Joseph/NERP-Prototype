const Network = require("../../consensus/raft/network");

const network = new Network();

console.log("\n==============================");
console.log("NERP INCIDENT DATABASE");
console.log("ATTACK: RAFT ELECTION STORM");
console.log("==============================");

console.log("\nFive Raft replicas across three regions.");
console.log("Region 1: 2 | Region 2: 2 | Region 3: 1");

console.log("\nNetwork instability begins.");
network.partition();

console.log("\nRepeated election attempts occur.");

for (let i = 1; i <= 5; i++) {
    console.log(`\nElection attempt ${i}:`);
    network.send(`RequestVote(term=${i})`);
}

console.log("\nNetwork instability continues.");
network.send("AppendEntries(term=5)");

console.log("\nLESSON:");
console.log("Repeated election attempts can prevent stable leadership.");
console.log("This affects availability and delays log replication.");