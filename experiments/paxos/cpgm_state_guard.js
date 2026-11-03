const Acceptor = require("../../consensus/paxos/acceptor");
const CPGM = require("./cpgm");

const cpgm = new CPGM();

const acceptor = new Acceptor("Region1-A1");

// Simulated trusted state from persistent recovery data
const trustedState = {
    highestPromised: 101,
    acceptedProposal: {
        number: 101,
        value: "Allocate ambulance to INC-001"
    }
};

// Current acceptor state before failure
acceptor.highestPromised = 101;
acceptor.acceptedProposal = {
    number: 101,
    value: "Allocate ambulance to INC-001"
};

console.log("\n=== NERP PAXOS STATE RECOVERY GUARD ===");
console.log("Service: Resource Allocation");
console.log("Acceptor: Region1-A1");

console.log("\n[1] Checking recovered acceptor state...");
cpgm.checkAcceptor(acceptor, trustedState);

// Simulate loss of persistent state
console.log("\n[2] Simulating acceptor state loss...");

acceptor.highestPromised = 0;
acceptor.acceptedProposal = null;

console.log("\n[3] CPGM performs a recovery check...");

const verified = cpgm.checkAcceptor(acceptor, trustedState);

console.log("\n[4] Attempting to use acceptor for a new proposal...");

if (verified && cpgm.canParticipate(acceptor)) {
    console.log("[PAXOS] Acceptor allowed to participate.");
} else {
    console.log("[PAXOS DENIED] Acceptor blocked by CPGM.");
}