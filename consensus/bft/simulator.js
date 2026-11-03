const Network = require("./network");
const QuorumEngine = require("./quorum");

class Simulator {

    constructor(nodes, customQuorumEngine = null, options = {}) {

        this.nodes = nodes;
        this.network = new Network();

        this.hardened = options.hardened || false;
        this.approved = options.approvedValidators || null; // 8.3
        this.revoked = new Set();                           // 8.2
        this.equivocationLog = [];                          // 8.1

        this.validatorCount = this.approved
            ? this.approved.length
            : nodes.length;

        this.quorum = customQuorumEngine ||
            new QuorumEngine(this.validatorCount);

        if (this.hardened) this.validateQuorum();           // 8.4
    }

    /* 8.4 Quorum sanity check */
    validateQuorum() {
        const minimum = Math.floor((2 * this.validatorCount) / 3) + 1;
        if (this.quorum.size() < minimum) {
            throw new Error(
                `Unsafe quorum size ${this.quorum.size()} (minimum ${minimum})`
            );
        }
    }

    /* 8.2 Key revocation */
    revoke(nodeId) {
        this.revoked.add(nodeId);
        console.log(`KEY REVOKED: ${nodeId}`);
    }

    /* 8.3 + 8.2 + signature check */
    acceptVote(vote) {

        if (this.approved && !this.approved.includes(vote.nodeId)) {
            console.log(`REJECTED ${vote.nodeId}: not an approved validator`);
            return false;
        }

        if (this.revoked.has(vote.nodeId)) {
            console.log(`REJECTED ${vote.nodeId}: key revoked`);
            return false;
        }

        const node = this.nodes.find(n => n.id === vote.nodeId);

        if (!node || !node.verifyVote(vote, () => node.secret)) {
            console.log(`REJECTED ${vote.nodeId}: bad signature`);
            return false;
        }

        return true;
    }

    /* 8.1 Equivocation detection */
    recordMessage(round, nodeId, value, recipient) {

        const entry = { round, nodeId, value, recipient };

        const conflict = this.equivocationLog.find(e =>
            e.round === round &&
            e.nodeId === nodeId &&
            e.value !== value
        );

        this.equivocationLog.push(entry);

        if (conflict) {
            console.log(
                `EQUIVOCATION DETECTED: ${nodeId} sent ${conflict.value} to ` +
                `${conflict.recipient} and ${value} to ${recipient} in round ${round}`
            );
            return true;
        }
        return false;
    }

    runConsensus(value) {

        console.log("\n====================");
        console.log(`Proposal: ${value}`);
        console.log("====================");

        const allVotes = [];

        for (const node of this.nodes) {
            const vote = node.vote(value);
            allVotes.push(vote);
            console.log(`${node.id} voted ${vote.value}`);
        }

        const votes = allVotes.filter(v =>
            !this.hardened || this.acceptVote(v)
        );

        const tally = {};
        for (const vote of votes) {
            tally[vote.value] = (tally[vote.value] || 0) + 1;
        }

        console.log("\nVote Tally");
        console.table(tally);
        console.log(`Required Quorum = ${this.quorum.size()}`);

        const winners = [];
        for (const candidate in tally) {
            if (this.quorum.reached(tally[candidate])) {
                winners.push({ value: candidate, votes: tally[candidate] });
            }
        }

        let certificate = null;

        if (winners.length === 0) {
            console.log("\nNO CONSENSUS");
        }
        else if (winners.length === 1) {
            console.log(`\nCOMMITTED: ${winners[0].value}`);

            /* 8.5 Commit certificate */
            certificate = {
                value: winners[0].value,
                signatures: votes
                    .filter(v => String(v.value) === winners[0].value)
                    .map(v => ({ nodeId: v.nodeId, signature: v.signature }))
            };

            console.log("\nCOMMIT CERTIFICATE");
            console.log(`Value = ${certificate.value}`);
            console.log("Signed by:");
            certificate.signatures.forEach(s => console.log(`  ${s.nodeId}`));
        }
        else {
            console.log("\nSAFETY VIOLATION!");
            console.log("Multiple values reached quorum:");
            console.table(winners);
        }

        return { proposal: value, votes, tally, winners, certificate };
    }
}

module.exports = Simulator;
