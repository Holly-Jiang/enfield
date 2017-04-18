#include "Analyser.h"
#include "Graph.h"
#include "Isomorphism.h"
#include "DynSolver.h"

#include <iostream>
#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <queue>

using namespace std;

enum Method M;

std::string PhysFilename;
std::string ProgFilename;

Graph* readGraph(string filename) {
    ifstream ifs(filename.c_str());

    int n;
    Graph *graph;

    ifs >> n;

    graph = new Graph(n);

    for (int u, v; ifs >> u >> v;)
        graph->putEdge(u, v);

    ifs.close();
    return graph;
}

int getFreeVertex(Graph &physGraph, vector<int> mapping) {
    int n = physGraph.size();

    bool assigned[n];
    for (int i = 0; i < n; ++i)
        assigned[i] = false;

    for (int i = 0, e = mapping.size(); i < e; ++i)
        if (mapping[i] != -1)
            assigned[mapping[i]] = true;

    for (int i = 0; i < n; ++i)
        if (!assigned[i])
            return i;

    return -1;
}

vector<int> getPath(Graph &physGraph, int u, int v) {
    vector<int> parent(physGraph.size(), -1);
    vector<bool> marked(physGraph.size(), false);

    queue<int> q;
    q.push(u);
    marked[u] = true;

    while (!q.empty()) {
        int x = q.front();
        q.pop();

        if (x == v) break;

        set<int> &succ = physGraph.succ(x);
        for (int k : succ) {
            if (!marked[k]) {
                q.push(k);
                marked[k] = true;
                parent[k] = x;
            }
        }
    }

    int x = v;
    vector<int> path;
    do {
        path.push_back(x);
        x = parent[x];
    } while (parent[x] != -1);

    return path;
}

void swapPath(Graph &physGraph, vector<int> &mapping, vector<int> path, ostream &out) {
    vector<int> assigned(physGraph.size(), -1);

    for (int i = 0, e = mapping.size(); i < e; ++i)
        if (mapping[i] != -1)
            assigned[mapping[i]] = i;

    for (int i = 1, e = path.size(); i < e; ++i) {
        // (u, v) in Phys
        int u = path[i-1], v = path[i];
        // (a, b) in Prog
        int a = assigned[u], b = assigned[v];

        out << "Swapping (" << a << ", " << b << ")" << endl;

        assigned[u] = b;
        assigned[v] = a;

        if (a != -1)
            mapping[a] = v;
        if (b != -1)
            mapping[b] = u;

        path[i-1] = v;
        path[i] = u;
    }
}

vector< vector<int> > mapForEach(Graph &physGraph, vector<int> mapping) {
    ifstream ifs(ProgFilename.c_str());

    int n;
    ifs >> n;

    vector< pair<int, int> > dependencies;
    for (int u, v; ifs >> u >> v;) {
        dependencies.push_back(pair<int, int>(u, v));
    }

    vector< vector<int> > mappings = { mapping };
    for (int t = 0, e = dependencies.size(); t < e; ++t) {
        pair<int, int> dep = dependencies[t];
        vector<int> current = mappings.back();

        int u = current[dep.first], v = current[dep.second];
        if (u == -1) {
            u = getFreeVertex(physGraph, current);
            current[dep.first] = u;
        }

        if (v == -1) {
            v = getFreeVertex(physGraph, current);
            current[dep.second] = v;
        }

        vector<int> path = getPath(physGraph, u, v);
        if (path.size() > 1) {
            swapPath(physGraph, current, path, cout);
            mappings.push_back(current);
        }

        cout << "Dep: " << dep.first << " -> " << dep.second << endl;

    }

    return mappings;
}

void printMapping(vector<int> &mapping) {
    cout << "Prog -> Phys" << endl;

    for (int i = 0; i < mapping.size(); ++i) {
        if (mapping[i] != -1)
            cout << i << " -> " << mapping[i] << endl;
    }
}

void readArgs(int argc, char **argv) {
    PhysFilename = argv[1];
    ProgFilename = argv[2];

    M = NONE;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-iso")) M = ISO;
        if (!strcmp(argv[i], "-dyn")) M = DYN;
    }

    if (M == NONE) M = DYN;
}

int main(int argc, char **argv) {
    readArgs(argc, argv);

    Graph *physGraph = readGraph(PhysFilename);
    physGraph->buildReverseGraph();
    physGraph->print();

    cout << endl;

    Graph *progGraph = readGraph(ProgFilename);
    progGraph->print();

    cout << endl;
    vector< vector<int> >  mappings;

    cout << "--------------------------------" << endl;
    vector<int> mapping;

    switch (M) {
        case ISO:
            {
                mapping = findIsomorphism(*physGraph, *progGraph);
                break;
            }

        case DYN:
            {
                mapping = dynsolve(*physGraph);
            }

        default:
            break;
    }

    printMapping(mapping);
    mappings = mapForEach(*physGraph, mapping);

    cout << "--------------------------------" << endl;
    for (int i = 0, e = mappings.size(); i < e; ++i)
        printMapping(mappings[i]);

    return 0;
}