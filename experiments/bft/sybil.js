const Node = require("../../consensus/bft/bftnode");

const nodes = [
    new Node("Region1-Validator-A"),
    new Node("Region1-Validator-B"),
    new Node("Region2-Validator-C"),
    new Node("Region3-Validator-D")
];

console.log("Sybil attack - Intelligence Distribution Service");
console.log("Original NERP validator network:");
console.table(nodes.map(n => n.id));

console.log("\nAttacker creates four fake validator identities.");

nodes.push(new Node("Region1-Validator-X1"));
nodes.push(new Node("Region2-Validator-X2"));
nodes.push(new Node("Region3-Validator-X3"));
nodes.push(new Node("Region3-Validator-X4"));

console.log("\nValidator network after Sybil attack:");
console.table(nodes.map(n => n.id));