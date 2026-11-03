const Quorum = require("../../consensus/paxos/quorum");

console.log("\n==============================");
console.log("NERP RESOURCE ALLOCATION");
console.log("ATTACK: QUORUM MISCONFIGURATION");
console.log("==============================");

const quorum = new Quorum(5);

console.log("\nFive acceptors across three regions:");
console.log("Region 1: A1, A2");
console.log("Region 2: A3, A4");
console.log("Region 3: A5");

console.log("\nCorrect majority:");
console.log(quorum.majority());

// Unsafe quorum modification
console.log("\nAttacker reduces the required quorum to 2.");
quorum.majority = () => 2;

console.log("\nModified majority:");
console.log(quorum.majority());

console.log("\nPossible conflicting quorums:");
console.log("Value A => A1, A2");
console.log("Value B => A4, A5");

console.log("\nWARNING:");
console.log("Quorum intersection is no longer guaranteed.");
console.log("Different values could be chosen.");