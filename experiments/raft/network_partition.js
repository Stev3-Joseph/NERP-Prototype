const Network = require("../../consensus/raft/network");

const network = new Network();

console.log("\n==============================");
console.log("NERP INCIDENT DATABASE");
console.log("ATTACK: RAFT NETWORK PARTITION");
console.log("==============================");

console.log("\nFive Raft replicas across three regions.");
console.log("Region 1: 2 | Region 2: 2 | Region 3: 1");

console.log("\nLeader communicating normally.");
network.send("AppendEntries(term=1)");

console.log("\nNetwork partition occurs.");
console.log("Leader messages cannot reach isolated replicas.");

network.partition();

network.send("AppendEntries(term=1)");
network.send("AppendEntries(instances=10)");

console.log("\nNetwork partition is repaired.");
console.log("Communication between replicas resumes.");

network.repair();

network.send("AppendEntries(term=1)");

console.log("\nLESSON:");
console.log("Network partitions disrupt replication and availability.");