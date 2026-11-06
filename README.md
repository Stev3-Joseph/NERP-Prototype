# NERP Prototype

A simple Node.js prototype for the National Emergency Response Platform (NERP).

## 1. Basic setup

Open PowerShell in the project folder:

Check that Node.js and npm are installed:

```powershell
node -v
npm -v
```

No additional npm packages are required for the JavaScript services. Run the commands below from the folder containing `package.json`.

## 2. Run the service tests

Each service has an implementation file and a test/demo file. Run an individual test:

```powershell
npm run test:auth
npm run test:incidents
npm run test:dispatch
npm run test:intelligence
npm run test:audit
npm run test:analytics
npm run test:governance
```

Run all service tests:

```powershell
npm test
```

## 3. Project structure

- **Services:** `auth/`, `incidents/`, `dispatch/`, `intelligence/`, `audit/`, `analytics/`, `governance/`
- **Datastores:** `datastore/` — JSON records for incidents, responders, dispatch, intelligence and audit
- **Consensus models:** `consensus/` — BFT, Paxos and Raft implementations
- **Experiments:** `experiments/` — attack and failure simulations for Part B
- **Parts C and D:** `part-c-d-implementation/` — C implementation for data classification, governance and access control.

The service test files demonstrate the corresponding service operations using the project data.

## 4. Run Part B consensus experiments

### BFT

```powershell
npm run bft:equivocation
npm run bft:key-theft
npm run bft:sybil
```

Run all BFT experiments:

```powershell
npm run bft:all
```

### Paxos

```powershell
npm run paxos:majority-loss
npm run paxos:quorum-misconfiguration
npm run paxos:state-loss
```

Run all Paxos experiments:

```powershell
npm run paxos:all
```

### Raft

```powershell
npm run raft:election-storm
npm run raft:leader-churn
npm run raft:network-partition
```

Run all Raft experiments:

```powershell
npm run raft:all
```

Run every listed Part B experiment:

```powershell
npm run partb
```

### Run the CPGM hardening experiment

The Cross-Protocol Governance Monitor (CPGM) demonstrates an additional security-hardening mechanism for Paxos.

It implements a Paxos State Recovery Guard that:

Compares a recovered acceptor's current state against a trusted reference state.

Detects mismatches in highestPromised and acceptedProposal.

Quarantines an acceptor when state inconsistency is detected.

Records the detected violation in datastore/audit.json.

Blocks the quarantined acceptor from participating in the experiment.

Run the experiment:
```powershell
npm run paxos:cpgm
```
## 5. Run Parts C and D

Run these commands from the project root in Linux or Ubuntu WSL. Use the Linux versions of Node.js and npm. The C/D implementation reads the shared JSON files in `datastore/`.

On Ubuntu, install the build and test dependencies if needed:

```bash
sudo apt install build-essential libssl-dev python3
```

Build the C/D executable and run the integration tests:

```bash
npm run partcd:build
npm run partcd:test
```

A successful test run reports `"status": "PASS"` and `"checks": 33`.
