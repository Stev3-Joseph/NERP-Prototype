# NERP Prototype

A simple Node.js prototype for the National Emergency Response Platform (NERP).

## 1. Basic setup

Open PowerShell in the project folder:

```powershell
cd "C:\MSc Cyber\Assesment 3\nerp-prototype"
```

Check that Node.js and npm are installed:

```powershell
node -v
npm -v
```

No additional packages are currently required. Run the commands below from the folder containing `package.json`.

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

## 5. Important notes

- Run commands from the project root, where `package.json` is located.
- Experiment commands execute the JavaScript files in `experiments/`.
- These are educational simulations. Their output demonstrates only the behaviour implemented by each experiment; do not assume a scenario proves full protocol-level behaviour unless the code actually models it.
- The `policy engine` folder is not included in the service test commands because no JavaScript implementation or test file was listed in the current project structure.
