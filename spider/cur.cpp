#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <limits>
#include <sstream>
#include <map>

using namespace std;

struct Edge {
    int destination;
    int distance;
};

struct Driver {
    int currentLocation;
    bool available;
};

struct Vertex {
    int id;
    int distance;
    bool operator>(const Vertex& other) const {
        return distance > other.distance;
    }
};

int V, E, D;
vector<vector<Edge>> graph;
vector<Driver> drivers;

void initializeGraph() {
    graph.resize(V + 1);
}

void addEdge(int source, int destination, int distance) {
    if (source <= V && destination <= V) {
        graph[source].push_back({destination, distance});
        graph[destination].push_back({source, distance});  // Assuming undirected graph
    }
}

vector<int> dijkstra(int src) {
    vector<int> distance(V + 1, numeric_limits<int>::max());
    priority_queue<Vertex, vector<Vertex>, greater<Vertex>> minHeap;

    distance[src] = 0;
    minHeap.push({src, 0});

    while (!minHeap.empty()) {
        Vertex current = minHeap.top();
        minHeap.pop();

        for (const Edge& edge : graph[current.id]) {
            int newDist = distance[current.id] + edge.distance;
            if (newDist < distance[edge.destination]) {
                distance[edge.destination] = newDist;
                minHeap.push({edge.destination, newDist});
            }
        }
    }

    return distance;
}

void handleOrder(int orderId, int src, int dst) {
    cout << "Order " << orderId << " from: " << src << endl;
    vector<int> distances = dijkstra(src);
    if (distances[dst] == numeric_limits<int>::max()) {
        cout << "No Way Home" << endl;
    } else {
        cout << "Order " << orderId << " distance: " << distances[dst] << endl;
    }
}

void dropDriver(int driverId, int location) {
    drivers[driverId].currentLocation = location;
    //cout << "Driver " << driverId << " dropped to location " << location << endl;
}

void completeOrder(int driverId) {
    drivers[driverId].available = true;
    //cout << "Driver " << driverId << " is now available" << endl;
}

int main() {
    ifstream inFile("input.csv");
    if (!inFile) {
        cerr << "Error opening input file." << endl;
        return 1;
    }

    string line;
    getline(inFile, line);
    istringstream iss(line);
    iss >> V >> E >> D;
    initializeGraph();
    drivers.resize(D + 1);

    int readEdges = 0;
    while (getline(inFile, line) && readEdges < E) {
        istringstream iss(line);
        string type;
        int a, b, c;
        iss >> type;
        if (type == "EDGE") {
            iss >> a >> b >> c;
            addEdge(a, b, c);
            readEdges++;
        }
    }

    while (getline(inFile, line)) {
        istringstream iss(line);
        string command;
        int id, a, b;
        iss >> command;
        if (command == "Order") {
            iss >> id >> a >> b;
            handleOrder(id, a, b);
        } else if (command == "Drop") {
            iss >> id >> a;
            dropDriver(id, a);
        } else if (command == "Complete") {
            iss >> id;
            completeOrder(id);
        }
    }

    inFile.close();
    return 0;
}
