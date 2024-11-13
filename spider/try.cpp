#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <queue>
#include <map>
#include <algorithm>
#include <climits>
using namespace std;

// 定義邊結構
struct Edge {
    int to, distance, capacity; // 目標頂點、距離、容量
};

// 定義訂單結構
struct Order {
    int id, src, ts, driverLocation, distance;
    bool waiting; // 訂單ID、來源頂點、佔用交通空間、司機位置、總距離、是否等待
};

// 定義司機結構
struct Driver {
    int location;
    bool available; // 位置、是否可用
};

// 圖的鄰接表表示
vector<vector<Edge>> graph;
// 每個頂點的司機列表
map<int, vector<Driver>> driversAtLocation;
// 活躍的訂單
map<int, Order> activeOrders;
// 圖的頂點數、邊數、司機數
int V, E, D;

// 添加邊
void addEdge(int s, int d, int dis, int t) {
    graph[s].push_back({d, dis, t}); // 添加邊到鄰接表
    graph[d].push_back({s, dis, t}); // 因為是無向圖，需要添加反向邊
}

// 找到最近的可用司機
int findNearestDriver(int src) {
    // 使用 BFS 來找尋最近的可用司機
    Driver driver;
    queue<int> q; // 用於 BFS 的隊列
    vector<bool> visited(V + 1, false); // 訪問標記
    q.push(src); // 起始頂點
    visited[src] = true; // 標記起始頂點為已訪問
    while (!q.empty()) {
        int u = q.front(); // 當前頂點
        q.pop(); // 移除隊首元素
        if (!driversAtLocation[u].empty()) { // 如果該頂點有司機
            cout << "Checking location " << u << " for available drivers." << endl;
            for (auto &driver : driversAtLocation[u]) { // 遍歷司機
            //for(int i = 0 ; i < D; i++){
                if (driver.available) { // 如果司機可用
                    cout << "Driver found at location " << u << "." << endl;
                    return u; // 返回司機位置
                }
            }
        }
        for (auto &edge : graph[u]) { // 遍歷相鄰頂點
            if (!visited[edge.to]) { // 如果相鄰頂點未被訪問
                q.push(edge.to); // 加入隊列
                visited[edge.to] = true; // 標記為已訪問
            }
        }
    }
    return -1; // 如果找不到司機，返回 -1
}

// 預留交通空間並找到最短路徑
bool reserveTrafficSpace(int src, int dst, int ts, vector<int>& path) {
    // 使用 Dijkstra 找尋最短路徑並嘗試預留交通空間
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<>> pq; // 最小堆
    vector<int> dist(V + 1, INT_MAX); // 距離陣列
    vector<int> prev(V + 1, -1); // 前驅陣列
    dist[src] = 0; // 起始頂點距離為 0
    pq.push({0, src}); // 將起始頂點加入堆

    while (!pq.empty()) {
        int d = pq.top().first; // 當前頂點的距離
        int u = pq.top().second; // 當前頂點
        pq.pop(); // 移除堆頂元素
        if (d > dist[u]) continue; // 如果當前距離大於已知距離，跳過
        for (auto &edge : graph[u]) { // 遍歷相鄰邊
            int v = edge.to; // 相鄰頂點
            int weight = edge.distance; // 邊的權重
            if (dist[u] + weight < dist[v] && edge.capacity >= ts) { // 如果新距離小於已知距離且邊容量足夠
                dist[v] = dist[u] + weight; // 更新距離
                prev[v] = u; // 設置前驅
                pq.push({dist[v], v}); // 將相鄰頂點加入堆
            }
        }
    }

    if (dist[dst] == INT_MAX) return false; // 如果找不到路徑，返回 false

    // 確認路徑是否可用，並預留交通空間
    int current = dst; // 從目標頂點開始
    while (prev[current] != -1) { // 逆向回溯路徑
        path.push_back(current); // 添加頂點到路徑
        int u = prev[current]; // 前驅頂點
        for (auto &edge : graph[u]) { // 遍歷邊
            if (edge.to == current) {
                edge.capacity -= ts; // 減少邊的容量
                break;
            }
        }
        for (auto &edge : graph[current]) { // 反向邊
            if (edge.to == u) {
                edge.capacity -= ts; // 減少反向邊的容量
                break;
            }
        }
        current = u; // 更新當前頂點
    }
    path.push_back(src); // 添加起始頂點到路徑
    reverse(path.begin(), path.end()); // 逆序路徑使其成為正向
    return true;
}

// 釋放交通空間
void releaseTrafficSpace(const vector<int>& path, int ts) {
    for (size_t i = 1; i < path.size(); ++i) { // 遍歷路徑
        int u = path[i - 1]; // 前一個頂點
        int v = path[i]; // 當前頂點
        for (auto &edge : graph[u]) { // 遍歷邊
            if (edge.to == v) {
                edge.capacity += ts; // 增加邊的容量
                break;
            }
        }
        for (auto &edge : graph[v]) { // 反向邊
            if (edge.to == u) {
                edge.capacity += ts; // 增加反向邊的容量
                break;
            }
        }
    }
}

// 處理新訂單
void processOrder(int id, int src, int ts) {
    int driverLocation = findNearestDriver(src); // 找到最近的可用司機
    if (driverLocation == -1) {
        cout << "Just walk. T-T" << endl; // 沒有可用司機
        return;
    }
    vector<int> path;
    if (!reserveTrafficSpace(driverLocation, src, ts, path)) { // 嘗試預留交通空間並找到路徑
        cout << "Just walk. T-T" << endl; // 沒有可用路徑
        return;
    }
    activeOrders[id] = {id, src, ts, driverLocation, 0, false}; // 記錄訂單信息
    for (auto &driver : driversAtLocation[driverLocation]) { // 標記司機為不可用
        if (driver.available) {
            driver.available = false;
            break;
        }
    }
    cout << "Order " << id << " from: " << driverLocation << endl; // 輸出訂單信息
}

// 處理訂單送達
void dropOrder(int id, int dst) {
    if (activeOrders.find(id) == activeOrders.end()) return; // 如果訂單不存在，返回
    auto &order = activeOrders[id];
    vector<int> path;
    if (!reserveTrafficSpace(order.src, dst, order.ts, path)) { // 嘗試預留交通空間並找到路徑
        cout << "No Way Home" << endl; // 沒有可用路徑
        order.waiting = true; // 訂單等待
        return;
    }
    int totalDistance = 0;
    for (size_t i = 1; i < path.size(); ++i) { // 計算總距離
        int u = path[i - 1];
        int v = path[i];
        for (auto &edge : graph[u]) { // 遍歷邊
            if (edge.to == v) {
                totalDistance += edge.distance; // 累加距離
                break;
            }
        }
    }
    releaseTrafficSpace(path, order.ts); // 釋放到取餐點的路徑空間
    cout << "Order " << id << " distance: " << totalDistance << endl; // 輸出總距離
    order.distance = totalDistance;
    order.src = dst; // 更新訂單的目標頂點
}

// 完成訂單
void completeOrder(int id) {
    if (activeOrders.find(id) == activeOrders.end()) return; // 如果訂單不存在，返回
    auto &order = activeOrders[id];
    vector<int> path;
    reserveTrafficSpace(order.driverLocation, order.src, order.ts, path); // 預留到取餐點的路徑空間
    releaseTrafficSpace(path, order.ts); // 釋放到取餐點的路徑空間
    // 司機變為可用狀態
    for (auto &driver : driversAtLocation[order.src]) {
        if (!driver.available) {
            driver.available = true;
            break;
        }
    }
    // 處理等待中的訂單
    vector<int> waitingOrders;
    for (auto &entry : activeOrders) { // 收集所有等待中的訂單
        if (entry.second.waiting) {
            waitingOrders.push_back(entry.first);
        }
    }
    sort(waitingOrders.begin(), waitingOrders.end()); // 按訂單ID排序
    for (int waitingId : waitingOrders) { // 處理等待中的訂單
        dropOrder(waitingId, activeOrders[waitingId].src);
    }
    activeOrders.erase(id); // 刪除完成的訂單
}

int main() {
    ifstream file("input.csv");
    string line;

    // 讀取第一行
    getline(file, line);
    stringstream ss(line);
    ss >> V >> E >> D;

    graph.resize(V + 1);

    // 讀取司機位置
    for (int i = 0; i < D; i++) {
        getline(file, line);
        stringstream ss(line);
        string place;
        int v, c;
        ss >> place >> v >> c;
        if (c > 0) { // 確保只有在司機數大於0時才初始化
            driversAtLocation[v].resize(c, {v, true});
            cout << "Initialized drivers at location " << v << " with " << c << " drivers." << endl; // 調試輸出
        }
    }

    // 讀取邊信息
    for (int i = 0; i < E; i++) {
        getline(file, line);
        stringstream ss(line);
        string edge;
        int s, d, dis, t;
        ss >> edge >> s >> d >> dis >> t;
        addEdge(s, d, dis, t);
    }

    // 跳過空行
    getline(file, line);

    // 讀取命令數
    getline(file, line);
    stringstream ss2(line);
    int C;
    ss2 >> C;

    // 讀取並處理命令
    for (int i = 0; i < C; i++) {
        getline(file, line);
        stringstream ss(line);
        string command;
        int id, param1, param2;
        ss >> command >> id >> param1;
        if (command == "Order") {
            ss >> param2;
            processOrder(id, param1, param2); // 處理新訂單
        } else if (command == "Drop") {
            dropOrder(id, param1); // 處理訂單送達
        } else if (command == "Complete") {
            completeOrder(id); // 完成訂單
        }
    }

    file.close();
    return 0;
}