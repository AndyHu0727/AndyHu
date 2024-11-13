#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <climits>
using namespace std;

struct Edge {
    int to, distance, capacity; // 目標頂點、距離、容量
    bool full; // 邊是否已滿載
    Edge(int t, int d, int c) : to(t), distance(d), capacity(c), full(false) {} // 構造函數
};

// 定義訂單結構
struct Order {
    int id, src, ts, driverLocation, distance;
    bool waiting; // 訂單ID、來源頂點、佔用交通空間、司機位置、總距離、是否等待
    vector<int> pathToSrc; // 儲存司機到取餐點的路徑
    vector<int> pathToDst; // 儲存取餐點到目的地的路徑
};

// 定義司機結構
struct Driver {
    int location;
    bool available; // 位置、是否可用

    bool operator==(const Driver& other) const {
        return location == other.location && available == other.available;
    }
};

// 圖的鄰接表表示
vector<vector<Edge>> graph;
// 每個頂點的司機列表
map<int, vector<Driver>> driversAtLocation;
// 活躍的訂單
map<int, Order> activeOrders;
// 等待處理的訂單
map<int, Order> waitingOrders;
// 圖的頂點數、邊數、司機數
int V, E, D;
vector<string> outputLogs; // 儲存最終的輸出

// 添加邊
void addEdge(int s, int d, int dis, int t) {
    graph[s].push_back(Edge(d, dis, t)); // 添加邊到鄰接表
    graph[d].push_back(Edge(s, dis, t)); // 因為是無向圖，需要添加反向邊
}

// 使用 Dijkstra 計算最短路徑
vector<int> dijkstra(int src, int ts) {
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    vector<int> dist(V + 1, INT_MAX);
    dist[src] = 0;
    pq.push(make_pair(0, src));

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        if (d > dist[u]) continue;
        for (int i = 0; i < graph[u].size(); ++i) {
            Edge &edge = graph[u][i];
            int v = edge.to;
            int weight = edge.distance;
            if (dist[u] + weight < dist[v] && edge.capacity >= ts) {
                dist[v] = dist[u] + weight;
                pq.push(make_pair(dist[v], v));
            }
        }
    }
    return dist;
}

// 預留交通空間並找到最短路徑
bool reserveTrafficSpace(int src, int dst, int ts, vector<int>& path) {
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    vector<int> dist(V + 1, INT_MAX);
    vector<int> prev(V + 1, -1);
    dist[src] = 0;
    pq.push(make_pair(0, src));

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        if (d > dist[u]) continue;
        for (int i = 0; i < graph[u].size(); ++i) {
            Edge &edge = graph[u][i];
            int v = edge.to;
            int weight = edge.distance;
            if (dist[u] + weight < dist[v] && edge.capacity >= ts) {
                dist[v] = dist[u] + weight;
                prev[v] = u;
                pq.push(make_pair(dist[v], v));
            }
        }
    }

    if (dist[dst] == INT_MAX) return false;

    int current = dst;
    while (prev[current] != -1) {
        path.push_back(current);
        int u = prev[current];
        for (int i = 0; i < graph[u].size(); ++i) {
            Edge &edge = graph[u][i];
            if (edge.to == current) {
                if (edge.capacity < ts) {
                    return false;
                }
                edge.capacity -= ts;
                edge.full = (edge.capacity == 0);
                break;
            }
        }
        for (int i = 0; i < graph[current].size(); ++i) {
            Edge &edge = graph[current][i];
            if (edge.to == u) {
                if (edge.capacity < ts) {
                    return false;
                }
                edge.capacity -= ts;
                edge.full = (edge.capacity == 0);
                break;
            }
        }
        current = u;
    }
    path.push_back(src);
    reverse(path.begin(), path.end());
    return true;
}

// 釋放交通空間
void releaseTrafficSpace(const vector<int>& path, int ts) {
    for (size_t i = 1; i < path.size(); ++i) {
        int u = path[i - 1];
        int v = path[i];
        for (int j = 0; j < graph[u].size(); ++j) {
            Edge &edge = graph[u][j];
            if (edge.to == v) {
                edge.capacity += ts;
                edge.full = false;
                break;
            }
        }
        for (int j = 0; j < graph[v].size(); ++j) {
            Edge &edge = graph[v][j];
            if (edge.to == u) {
                edge.capacity += ts;
                edge.full = false;
                break;
            }
        }
    }
}

// 找到最近的可用司機
int findNearestDriver(int src, int ts, int& distToSrc, vector<int>& pathToSrc) {
    vector<int> dist = dijkstra(src, ts);
    int minDist = INT_MAX;
    int bestLocation = -1;

    for (const auto& entry : driversAtLocation) {
        int location = entry.first;
        for (const auto& driver : entry.second) {
            if (driver.available) {
                vector<int> path;
                if (reserveTrafficSpace(driver.location, src, ts, path)) {
                    int totalDistance = 0;
                    for (size_t i = 1; i < path.size(); ++i) {
                        int u = path[i - 1];
                        int v = path[i];
                        for (int j = 0; j < graph[u].size(); ++j) {
                            Edge &edge = graph[u][j];
                            if (edge.to == v) {
                                totalDistance += edge.distance;
                                break;
                            }
                        }
                    }
                    if (totalDistance < minDist) {
                        minDist = totalDistance;
                        bestLocation = driver.location;
                        pathToSrc = path;
                    }
                    releaseTrafficSpace(path, ts);
                }
            }
        }
    }
    distToSrc = minDist;
    return bestLocation;
}

// 用於替換日志中的某個項目
void replaceLog(const string& searchStr, const string& replaceStr) {
    for (auto& log : outputLogs) {
        if (log.find(searchStr) != string::npos) {
            log = replaceStr;
            return;
        }
    }
    outputLogs.push_back(replaceStr);
}

// 處理新訂單
void processOrder(int id, int src, int ts) {
    int distToSrc;
    vector<int> pathToSrc;
    int driverLocation = findNearestDriver(src, ts, distToSrc, pathToSrc);
    if (driverLocation == -1) {
        outputLogs.push_back("No Way Home");
        waitingOrders[id] = (Order){id, src, ts, -1, 0, true, {}, {}};
        return;
    }

    if (!reserveTrafficSpace(driverLocation, src, ts, pathToSrc)) {
        outputLogs.push_back("No Way Home");
        waitingOrders[id] = (Order){id, src, ts, -1, 0, true, {}, {}};
        return;
    }

    replaceLog("Order " + to_string(id) + " from:", "Order " + to_string(id) + " from: " + to_string(driverLocation));
    activeOrders[id] = (Order){id, src, ts, driverLocation, distToSrc, false, pathToSrc, {}};
    for (auto& driver : driversAtLocation[driverLocation]) {
        if (driver.available) {
            driver.available = false;
            break;
        }
    }
}

// 處理訂單送達
bool dropOrder(int id, int dst) {
    if (activeOrders.find(id) == activeOrders.end() && waitingOrders.find(id) == waitingOrders.end()) return false;

    Order order = activeOrders.find(id) != activeOrders.end() ? activeOrders[id] : waitingOrders[id];
    vector<int> pathToDst;

    if (!reserveTrafficSpace(order.src, dst, order.ts, pathToDst)) {
        outputLogs.push_back("No Way Home");
        order.waiting = true;
        waitingOrders[id] = order;
        return false;
    }

    order.waiting = false;
    order.pathToDst = pathToDst;
    activeOrders[id] = order;
    waitingOrders.erase(id);

    // 計算總距離（司機到取餐點的距離 + 取餐點到目的地的距離）
    int totalDistance = 0;
    // 計算司機從原位置到取餐點的距離
    for (size_t i = 1; i < order.pathToSrc.size(); ++i) {
        int u = order.pathToSrc[i - 1];
        int v = order.pathToSrc[i];
        for (int j = 0; j < graph[u].size(); ++j) {
            Edge &edge = graph[u][j];
            if (edge.to == v) {
                totalDistance += edge.distance;
                break;
            }
        }
    }
    // 計算取餐點到目的地的距離
    for (size_t i = 1; i < pathToDst.size(); ++i) {
        int u = pathToDst[i - 1];
        int v = pathToDst[i];
        for (int j = 0; j < graph[u].size(); ++j) {
            Edge &edge = graph[u][j];
            if (edge.to == v) {
                totalDistance += edge.distance;
                break;
            }
        }
    }
    order.distance = totalDistance;
    order.src = dst;

    for (auto& driver : driversAtLocation[order.driverLocation]) {
        if (!driver.available) {
            driver.location = dst;
            driver.available = false;
            driversAtLocation[dst].push_back(driver);
            driversAtLocation[order.driverLocation].erase(
                remove(driversAtLocation[order.driverLocation].begin(), driversAtLocation[order.driverLocation].end(), driver),
                driversAtLocation[order.driverLocation].end());
            break;
        }
    }

    replaceLog("Order " + to_string(id) + " distance:", "Order " + to_string(id) + " distance: " + to_string(order.distance));
    return true;
}

// 完成訂單
void completeOrder(int id) {
    if (activeOrders.find(id) == activeOrders.end()) return;
    Order &order = activeOrders[id];

    releaseTrafficSpace(order.pathToSrc, order.ts);
    releaseTrafficSpace(order.pathToDst, order.ts);

    for (auto& driver : driversAtLocation[order.src]) {
        if (!driver.available) {
            driver.available = true;
            break;
        }
    }
    activeOrders.erase(id);

    vector<int> waitingOrderIds;
    for (const auto& entry : waitingOrders) {
        waitingOrderIds.push_back(entry.first);
    }
    sort(waitingOrderIds.begin(), waitingOrderIds.end());
    for (int waitingId : waitingOrderIds) {
        processOrder(waitingId, waitingOrders[waitingId].src, waitingOrders[waitingId].ts);
        dropOrder(waitingId, waitingOrders[waitingId].src);
    }
}

int main() {
    ifstream file("input.csv");
    string line;

    getline(file, line);
    stringstream ss(line);
    ss >> V >> E >> D;

    graph.resize(V + 1);

    for (int i = 0; i < D; i++) {
        getline(file, line);
        stringstream ss(line);
        string place;
        int v, c;
        ss >> place >> v >> c;
        if (c > 0) {
            driversAtLocation[v].resize(c, (Driver){v, true});
        }
    }

    for (int i = 0; E > 0 && i < E; i++) {
        getline(file, line);
        stringstream ss(line);
        string edge;
        int s, d, dis, t;
        ss >> edge >> s >> d >> dis >> t;
        addEdge(s, d, dis, t);
    }

    getline(file, line);

    getline(file, line);
    stringstream ss2(line);
    int C;
    ss2 >> C;

    for (int i = 0; i < C; i++) {
        getline(file, line);
        stringstream ss(line);
        string command;
        int id, param1, param2;
        ss >> command >> id;
        if (command == "Order") {
            ss >> param1 >> param2;
            processOrder(id, param1, param2);
        } else if (command == "Drop") {
            ss >> param1;
            dropOrder(id, param1);
        } else if (command == "Complete") {
            completeOrder(id);
        }
    }

    // for (const auto& entry : activeOrders) {
    //     int id = entry.first;
    //     const Order& order = entry.second;
    //     replaceLog("Order " + to_string(id) + " from:", "Order " + to_string(id) + " from: " + to_string(order.driverLocation));
    //     replaceLog("Order " + to_string(id) + " distance:", "Order " + to_string(id) + " distance: " + to_string(order.distance));
    // }

    for (const auto& log : outputLogs) {
        cout << log << endl;
    }

    file.close();
    return 0;
}
