const Node = require("../../consensus/raft/node");
const Cluster = require("../../consensus/raft/cluster");
const Simulator = require("../../consensus/raft/simulator");
const Network = require("../../consensus/raft/network");

const nodes = [
    new Node("Region1-Replica-A"),
    new Node("Region1-Replica-B"),
    new Node("Region2-Replica-C"),
    new Node("Region2-Replica-D"),
    new Node("Region3-Replica-E")
];

const cluster = new Cluster(nodes);
const simulator = new Simulator(cluster, new Network());

console.log("\n==============================");
console.log("NERP INCIDENT DATABASE");
console.log("ATTACK: RAFT LEADERSHIP CHURN");
console.log("==============================");

console.log("\nFive replicas across three regions.");
console.log("Repeated leadership changes begin.");

const leaders = [nodes[0], nodes[2], nodes[1], nodes[3], nodes[4]];

leaders.forEach((leader, index) => {
    console.log(`\nElection ${index + 1}: ${leader.id} becomes leader.`);
    simulator.electLeader(leader);
});

console.log("\nLESSON:");
console.log("Frequent leadership changes disrupt stable coordination.");
console.log("Log replication and incident updates may be delayed.");