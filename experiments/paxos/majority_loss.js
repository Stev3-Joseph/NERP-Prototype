const Proposer = require("../../consensus/paxos/proposer");
const Acceptor = require("../../consensus/paxos/acceptor");
const Learner = require("../../consensus/paxos/learner");
const Simulator = require("../../consensus/paxos/simulator");

const proposer = new Proposer("NERP-Resource-Proposer");

const acceptors = [
    new Acceptor("Region1-A1"),
    new Acceptor("Region1-A2"),
    new Acceptor("Region2-A1"),
    new Acceptor("Region2-A2"),
    new Acceptor("Region3-A1")
];

const learners = [
    new Learner("NERP-Resource-Learner")
];

console.log("\n==============================");
console.log("NERP RESOURCE ALLOCATION");
console.log("ATTACK: PAXOS MAJORITY LOSS");
console.log("==============================");

console.log("\nFive acceptors are distributed across three regions.");
console.log("Required quorum: 3");

// Region 1 and one acceptor in Region 2 fail
console.log("\nRegion 1 goes offline.");
acceptors[0].fail();
acceptors[1].fail();

console.log("One acceptor in Region 2 also fails.");
acceptors[2].fail();

console.log("\nOnly two acceptors remain operational.");
console.log("The required quorum of three cannot be reached.");

const simulator = new Simulator(proposer, acceptors, learners);

simulator.run("instances=10");