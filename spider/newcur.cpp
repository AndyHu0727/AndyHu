#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <climits>
using namespace std;

#define Mode 0 // 定義模式：0為從CSV讀取，1為從控制台讀取

struct Edge {
    int to, distance, capacity; // 邊的終點、距離和容量
    bool full; // 邊是否滿載的標志
    Edge(int t, int d, int c) : to(t), distance(d), capacity(c), full(false) {} // 構造函數
};

// 定義訂單結構
struct Order {
    int id, src, ts, driverLocation, distance;
    bool waiting; // 訂單的基本屬性，包括ID，起始點，交通空間，司機位置，總距離和等待狀態
    vector<int> pathToSrc; // 記錄司機到取餐點的路徑
    vector<int> pathToDst; // 記錄取餐點到目的地的路徑
};

// 定義司機結構
struct Driver {
    int location;
    bool available; // 司機位置和可用性
    bool operator==(const Driver& other) const {
        return location == other.location && available == other.available;
    }
};

// 全局變量定義
vector<vector<Edge>> graph; // 圖的鄰接表表示
map<int, vector<Driver>> driversAtLocation; // 各頂點的司機列表
map<int, Order> activeOrders; // 活躍訂單列表
map<int, Order> waitingOrders; // 等待中的訂單列表
int V, E, D; // 頂點數，邊數，司機數
vector<string> outputLogs; // 用於儲存最終輸出的日誌

// 函數定義
void addEdge(int s, int d, int dis, int t) {
    graph[s].push_back(Edge(d, dis, t)); // 添加正向邊
    graph[d].push_back(Edge(s, dis, t)); // 添加反向邊，因為是無向圖
}

vector<int> dijkstra(int src, int ts) {
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq; // 最小堆實現Dijkstra算法
    vector<int> dist(V + 1, INT_MAX); // 距離陣列，初始化為最大值
    dist[src] = 0;
    pq.push(make_pair(0, src));

    while (!pq.empty()) {
        int d = pq.top().first;
        int u = pq.top().second;
        pq.pop();
        if (d > dist[u]) continue; // 當前點的距離如果大於已知最短距離則跳過
        for (int i = 0; i < graph[u].size(); ++i) {
            Edge &edge = graph[u][i];
            int v = edge.to;
            int weight = edge.distance;
            if (dist[u] + weight < dist[v] && edge.capacity >= ts) { // 檢查容量並更新距離
                dist[v] = dist[u] + weight;
                pq.push(make_pair(dist[v], v));
            }
        }
    }
    return dist;
}

bool reserveTrafficSpace(int src, int dst, int ts, vector<int>& path) {
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    vector<int> dist(V + 1, INT_MAX); // 距離陣列
    vector<int> prev(V + 1, -1); // 前驅節點陣列
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

    if (dist[dst] == INT_MAX) return false; // 如果沒有找到路徑，返回false

    int current = dst;
    while (prev[current] != -1) {
        path.push_back(current);
        int u = prev[current];
        for (int i = 0; i < graph[u].size(); ++i) {
            Edge &edge = graph[u][i];
            if (edge.to == current) {
                if (edge.capacity < ts) return false; // 檢查容量是否足夠
                edge.capacity -= ts; // 預留容量
                edge.full = (edge.capacity == 0); // 更新邊的滿載標志
                break;
            }
        }
        for (int i = 0; i < graph[current].size(); ++i) {
            Edge &edge = graph[current][i];
            if (edge.to == u) {
                if (edge.capacity < ts) return false;
                edge.capacity -= ts;
                edge.full = (edge.capacity == 0);
                break;
            }
        }
        current = u;
    }
    path.push_back(src);
    reverse(path.begin(), path.end()); // 反轉路徑以獲得正確的方向
    return true; // 成功預留交通空間
}

void releaseTrafficSpace(const vector<int>& path, int ts) {
    for (size_t i = 1; i < path.size(); ++i) {
        int u = path[i - 1];
        int v = path[i];
        for (int j = 0; j < graph[u].size(); ++j) {
            Edge &edge = graph[u][j];
            if (edge.to == v) {
                edge.capacity += ts; // 釋放預留的容量
                edge.full = false; // 更新邊的滿載標志
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

int findNearestDriver(int src, int ts, int& distToSrc, vector<int>& pathToSrc) {
    vector<int> dist = dijkstra(src, ts); // 使用Dijkstra算法找到從src到所有點的最短距離
    int minDist = INT_MAX; // 最小距離初始設為無窮大
    int bestLocation = -1; // 最佳司機位置

    for (const auto& entry : driversAtLocation) { // 遍歷所有司機的位置
        int location = entry.first;
        for (const auto& driver : entry.second) { // 遍歷該位置的所有司機
            if (driver.available) {
                vector<int> path;
                if (reserveTrafficSpace(driver.location, src, ts, path)) { // 嘗試從司機位置到src預留交通空間
                    int totalDistance = 0;
                    for (size_t i = 1; i < path.size(); ++i) { // 計算路徑總距離
                        int u = path[i - 1];
                        int v = path[i];
                        for (int j = 0; j < graph[u].size(); ++j) {
                            Edge &edge = graph[u][j];
                            if (edge.to == v) {
                                totalDistance += edge.distance; // 累加距離
                                break;
                            }
                        }
                    }
                    if (totalDistance < minDist) {
                        minDist = totalDistance; // 更新最小距離
                        bestLocation = driver.location; // 更新最佳司機位置
                        pathToSrc = path; // 更新路徑
                    }
                    releaseTrafficSpace(path, ts); // 釋放預留的交通空間
                }
            }
        }
    }
    distToSrc = minDist; // 更新到src的距離
    return bestLocation; // 返回最佳司機位置
}

void replaceLog(const string& searchStr, const string& replaceStr) {
    for (auto& log : outputLogs) { // 遍歷所有日誌
        if (log.find(searchStr) != string::npos) {
            log = replaceStr; // 如果找到相應日誌，則替換
            return;
        }
    }
    outputLogs.push_back(replaceStr); // 如果沒找到，則添加新日誌
}

void processOrder(int id, int src, int ts) {
    int distToSrc;
    vector<int> pathToSrc;
    int driverLocation = findNearestDriver(src, ts, distToSrc, pathToSrc); // 尋找最近的可用司機
    if (driverLocation == -1) { // 如果沒有可用司機
        outputLogs.push_back("No Way Home"); // 輸出無法送達的信息
        waitingOrders[id] = (Order){id, src, ts, -1, 0, true, {}, {}}; // 將訂單添加到等待列表
        return;
    }

    if (!reserveTrafficSpace(driverLocation, src, ts, pathToSrc)) { // 如果無法預留交通空間
        outputLogs.push_back("No Way Home"); // 輸出無法送達的信息
        waitingOrders[id] = (Order){id, src, ts, -1, 0, true, {}, {}}; // 將訂單添加到等待列表
        return;
    }

    replaceLog("Order " + to_string(id) + " from:", "Order " + to_string(id) + " from: " + to_string(driverLocation)); // 替換或添加訂單起始司機位置的日誌
    activeOrders[id] = (Order){id, src, ts, driverLocation, distToSrc, false, pathToSrc, {}}; // 將訂單添加到活躍訂單列表
    for (auto& driver : driversAtLocation[driverLocation]) { // 遍歷司機位置
        if (driver.available) { // 找到可用司機
            driver.available = false; // 將司機標記為忙碌
            break;
        }
    }
}

bool dropOrder(int id, int dst) {
    if (activeOrders.find(id) == activeOrders.end() && waitingOrders.find(id) == waitingOrders.end()) return false; // 如果訂單不在活躍或等待列表中，返回false
    vector<int> pathToDst;
    Order order = activeOrders.find(id) != activeOrders.end() ? activeOrders[id] : waitingOrders[id]; // 獲取訂單信息

    if (!reserveTrafficSpace(order.src, dst, order.ts, pathToDst)) { // 如果無法預留從起始點到目的地的交通空間
        outputLogs.push_back("No Way Home"); // 輸出無法送達的信息
        order.waiting = true; // 訂單繼續等待
        waitingOrders[id] = order; // 更新等待訂單信息
        return false; // 返回false表示處理失敗
    }

    order.waiting = false; // 訂單不再等待
    order.pathToDst = pathToDst; // 更新訂單的目的地路徑
    activeOrders[id] = order; // 更新活躍訂單信息
    waitingOrders.erase(id); // 從等待列表中刪除訂單

    int totalDistance = order.distance; // 從訂單信息中獲取已經累計的距離
    for (size_t i = 1; i < pathToDst.size(); ++i) { // 遍歷目的地路徑
        int u = pathToDst[i - 1];
        int v = pathToDst[i];
        for (int j = 0; j < graph[u].size(); ++j) { // 遍歷相鄰邊
            Edge &edge = graph[u][j];
            if (edge.to == v) { // 如果找到目的地路徑上的邊
                totalDistance += edge.distance; // 累加距離
                break;
            }
        }
    }
    order.distance = totalDistance; // 更新訂單的總距離
    order.src = dst; // 更新訂單的當前位置為目的地
    for (auto& driver : driversAtLocation[order.driverLocation]) { // 遍歷司機位置
        if (!driver.available) { // 如果司機忙碌
            driver.location = dst; // 更新司機位置為目的地
            driver.available = false; // 確保司機狀態為忙碌
            driversAtLocation[dst].push_back(driver); // 將司機添加到新位置的司機列表
            driversAtLocation[order.driverLocation].erase( // 從原位置的司機列表中移除司機
                remove(driversAtLocation[order.driverLocation].begin(), driversAtLocation[order.driverLocation].end(), driver),
                driversAtLocation[order.driverLocation].end());
            break;
        }
    }
    outputLogs.push_back("Order " + to_string(id) + " distance: " + to_string(totalDistance)); // 輸出訂單的總距離
    //replaceLog("Order " + to_string(id) + " distance:", "Order " + to_string(id) + " distance: " + to_string(order.distance));
    
    return true; // 返回true表示處理成功
}

void completeOrder(int id) {
    if (activeOrders.find(id) == activeOrders.end()) return; // 如果訂單不在活躍列表中，直接返回
    Order &order = activeOrders[id]; // 獲取訂單信息

    releaseTrafficSpace(order.pathToSrc, order.ts); // 釋放司機到取餐點的路徑上的交通空間
    releaseTrafficSpace(order.pathToDst, order.ts); // 釋放取餐點到目的地的路徑上的交通空間

    for (auto& driver : driversAtLocation[order.src]) { // 遍歷司機位置
        if (!driver.available) { // 如果司機忙碌
            driver.available = true; // 將司機標記為可用
            break;
        }
    }
    activeOrders.erase(id); // 從活躍訂單列表中刪除訂單

    vector<int> waitingOrderIds; // 儲存等待中的訂單ID
    for (const auto& entry : waitingOrders) { // 遍歷等待中的訂單
        waitingOrderIds.push_back(entry.first);
    }
    sort(waitingOrderIds.begin(), waitingOrderIds.end()); // 對等待訂單ID進行排序
    for (int waitingId : waitingOrderIds) { // 遍歷排序後的等待訂單ID
        processOrder(waitingId, waitingOrders[waitingId].src, waitingOrders[waitingId].ts); // 處理每個等待中的訂單
        dropOrder(waitingId, waitingOrders[waitingId].ts); // 嘗試完成每個等待中的訂單的送達
    }
}
#if Mode
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
#endif

#if !Mode
int main() {
    // 初始化變量
    string line;

    // 讀取第一行：圖的頂點數、邊數、司機數
    getline(cin, line);
    stringstream ss(line);
    ss >> V >> E >> D;

    graph.resize(V + 1);

    // 讀取司機位置
    for (int i = 0; i < D; i++) {
        getline(cin, line);
        stringstream ss(line);
        string place;
        int v, c;
        ss >> place >> v >> c;
        if (c > 0) {
            driversAtLocation[v].resize(c, (Driver){v, true});
        }
    }

    // 讀取邊信息
    for (int i = 0; E > 0 && i < E; i++) {
        getline(cin, line);
        stringstream ss(line);
        string edge;
        int s, d, dis, t;
        ss >> edge >> s >> d >> dis >> t;
        addEdge(s, d, dis, t);
    }

    // 跳過空行
    getline(cin, line);

    // 讀取命令數
    getline(cin, line);
    stringstream ss2(line);
    int C;
    ss2 >> C;

    // 讀取並處理命令
    for (int i = 0; i < C; i++) {
        getline(cin, line);
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

    // 輸出所有日志
    for (const auto& log : outputLogs) {
        cout << log << endl;
    }

    return 0;
}

#endif"
