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
// 圖的頂點數、邊數、司機數
int V, E, D;
int plusdist = 0;
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
            if (dist[u] + weight < dist[v] && edge.capacity >= ts) { // 檢查邊的容量是否足夠
                dist[v] = dist[u] + weight;
                pq.push(make_pair(dist[v], v));
            }
        }
    }

    return dist;
}

// 預留交通空間並找到最短路徑
bool reserveTrafficSpace(int src, int dst, int ts, vector<int>& path) {
    // 使用 Dijkstra 找尋最短路徑並嘗試預留交通空間
    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq; // 最小堆
    vector<int> dist(V + 1, INT_MAX); // 距離陣列
    vector<int> prev(V + 1, -1); // 前驅陣列
    dist[src] = 0; // 起始頂點距離為 0
    pq.push(make_pair(0, src)); // 將起始頂點加入堆

    while (!pq.empty()) {
        int d = pq.top().first; // 當前頂點的距離
        int u = pq.top().second; // 當前頂點
        pq.pop(); // 移除堆頂元素
        if (d > dist[u]) continue; // 如果當前距離大於已知距離，跳過
        for (int i = 0; i < graph[u].size(); ++i) { // 遍歷相鄰邊
            Edge &edge = graph[u][i];
            int v = edge.to; // 相鄰頂點
            int weight = edge.distance; // 邊的權重
            if (dist[u] + weight < dist[v] && edge.capacity >= ts) { // 如果新距離小於已知距離且邊容量足夠
                dist[v] = dist[u] + weight; // 更新距離
                prev[v] = u; // 設置前驅
                pq.push(make_pair(dist[v], v)); // 將相鄰頂點加入堆
            }
        }
    }

    if (dist[dst] == INT_MAX) return false; // 如果找不到路徑，返回 false

    // 確認路徑是否可用，並預留交通空間
    int current = dst; // 從目標頂點開始
    while (prev[current] != -1) { // 逆向回溯路徑
        path.push_back(current); // 添加頂點到路徑
        int u = prev[current]; // 前驅頂點
        for (int i = 0; i < graph[u].size(); ++i) { // 遍歷邊
            Edge &edge = graph[u][i];
            if (edge.to == current) {
                if (edge.capacity < ts) { // 檢查邊的容量是否足夠
                    return false;
                }
                edge.capacity -= ts; // 減少邊的容量
                edge.full = (edge.capacity == 0); // 更新邊的滿載狀態
                break;
            }
        }
        for (int i = 0; i < graph[current].size(); ++i) { // 反向邊
            Edge &edge = graph[current][i];
            if (edge.to == u) {
                if (edge.capacity < ts) { // 檢查反向邊的容量是否足夠
                    return false;
                }
                edge.capacity -= ts; // 減少反向邊的容量
                edge.full = (edge.capacity == 0); // 更新反向邊的滿載狀態
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
        for (int j = 0; j < graph[u].size(); ++j) { // 遍歷邊
            Edge &edge = graph[u][j];
            if (edge.to == v) {
                edge.capacity += ts; // 增加邊的容量
                edge.full = false; // 更新邊的滿載狀態
                break;
            }
        }
        for (int j = 0; j < graph[v].size(); ++j) { // 反向邊
            Edge &edge = graph[v][j];
            if (edge.to == u) {
                edge.capacity += ts; // 增加反向邊的容量
                edge.full = false; // 更新反向邊的滿載狀態
                break;
            }
        }
    }
}

// 找到最近的可用司機
int findNearestDriver(int src, int ts) {
    vector<int> dist = dijkstra(src, ts); // 使用 Dijkstra 演算法計算從起點 src 到所有其他頂點的最短距離
    int minDist = INT_MAX; // 設定初始最小距離為無限大
    int bestLocation = -1; // 設定初始最佳位置為 -1

    for (int u = 0; u < driversAtLocation.size(); u++) { // 遍歷所有司機的位置
        if (!driversAtLocation[u].empty()) { // 如果該位置有司機
            for (int i = 0; i < driversAtLocation[u].size(); ++i) { // 遍歷該位置的所有司機
                Driver &driver = driversAtLocation[u][i]; // 獲取司機引用
                if (driver.location == src) { // 如果司機的位置與起點相同
                    plusdist = 0; // 將 plusdist 設為 0，表示司機已在起點
                    bestLocation = driver.location; // 更新最佳位置為司機的位置
                    return bestLocation;
                }
                vector<int> path;
                for (int k = 0; k < graph[driver.location].size(); ++k) {
                    if (driver.available && dist[u] < minDist && ts <= graph[driver.location][k].capacity) {
                        minDist = dist[u]; // 更新最小距離
                        bestLocation = driversAtLocation[u][i].location; // 更新最佳位置為該司機的位置
                        break; // 跳出內部的 for 循環
                    }
                }
            }
        }
    }
    return bestLocation;
}

// 處理新訂單
void processOrder(int id, int src, int ts) {
    int driverLocation = findNearestDriver(src, ts); // 找到最近的可用司機
    if (driverLocation == -1) {
        cout << "No Way Home" << endl; // 沒有可用司機
        return;
    }
    vector<int> path;
    if (!reserveTrafficSpace(driverLocation, src, ts, path)) { // 嘗試預留交通空間並找到路徑
        cout << "No Way Home" << endl; // 沒有可用路徑
        activeOrders[id].waiting = true; // 訂單等待
        return;
    }
    activeOrders[id].waiting = false;
    activeOrders[id] = (Order){id, src, ts, driverLocation, 0, false}; // 記錄訂單信息
    for (int i = 0; i < driversAtLocation[driverLocation].size(); ++i) 
    { // 標記司機為不可用
        Driver &driver = driversAtLocation[driverLocation][i];
        if (driver.available)
        {
            driver.available = false;
            break;
        }
    }
    if (driverLocation != src) 
    {
        for (int i = 0; i < graph[src].size(); ++i) 
        {
            if (graph[src][i].to == driverLocation) 
            {
                plusdist = graph[src][i].distance;
                graph[src][i].full = true;
                break;
            }
        }
    } 
    else 
    {
        plusdist = 0;
    }
    cout << "Order " << id << " from: " << driverLocation << endl; // 輸出訂單信息
}

// 處理訂單送達
bool dropOrder(int id, int dst) {
    if (activeOrders.find(id) == activeOrders.end()) return false; // 如果訂單不存在，返回 false
    Order &order = activeOrders[id];
    vector<int> path;

    if (!reserveTrafficSpace(order.src, dst, order.ts, path)) { // 嘗試預留交通空間並找到路徑
        cout << "No Way Home" << endl; // 沒有可用路徑
        order.waiting = true; // 訂單等待
        return false; // 返回 false
    }
    order.waiting = false; // 訂單不再等待

    int totalDistance = 0;
    for (size_t i = 1; i < path.size(); ++i) { // 計算總距離
        int u = path[i - 1];
        int v = path[i];
        for (int j = 0; j < graph[u].size(); ++j) { // 遍歷邊
            Edge &edge = graph[u][j];
            if (edge.to == v) {
                totalDistance += edge.distance; // 累加距離
                break;
            }
        }
    }
    //releaseTrafficSpace(path, order.ts); // 釋放到取餐點的路徑空間
    //cout << "plusdist = " << plusdist << endl;
    cout << "Order " << id << " distance: " << totalDistance + plusdist << endl; // 輸出總距離
    order.distance = totalDistance;
    order.src = dst; // 更新訂單的目標頂點

    // 更新司機位置
    for (int i = 0; i < driversAtLocation[order.driverLocation].size(); ++i) {
        Driver &driver = driversAtLocation[order.driverLocation][i];
        if (!driver.available) {
            driversAtLocation[dst].push_back(driver); // 將司機移動到新位置
            driversAtLocation[order.driverLocation].erase(
                remove(driversAtLocation[order.driverLocation].begin(), driversAtLocation[order.driverLocation].end(), driver),
                driversAtLocation[order.driverLocation].end());
            break;
        }
    }

    return true; // 返回 true，表示成功處理訂單
}

// 完成訂單
void completeOrder(int id) {
    if (activeOrders.find(id) == activeOrders.end()) return; // 如果訂單不存在，返回
    Order &order = activeOrders[id];
    vector<int> path;
    reserveTrafficSpace(order.driverLocation, order.src, order.ts, path); // 預留到取餐點的路徑空間
    releaseTrafficSpace(path, order.ts); // 釋放到取餐點的路徑空間
    // 司機變為可用狀態
    for (int i = 0; i < driversAtLocation[order.src].size(); ++i) {
        Driver &driver = driversAtLocation[order.src][i];
        if (!driver.available) {
            driver.available = true;
            break;
        }
    }
    // 處理等待中的訂單
    vector<int> waitingOrders;
    for (map<int, Order>::iterator it = activeOrders.begin(); it != activeOrders.end(); ++it) { // 收集所有等待中的訂單
        if (it->second.waiting) {
            waitingOrders.push_back(it->first);
        }
    }
    sort(waitingOrders.begin(), waitingOrders.end()); // 按訂單ID排序
    for (int i = 0; i < waitingOrders.size(); ++i) { // 處理等待中的訂單
        int waitingId = waitingOrders[i];
        if (dropOrder(waitingId, activeOrders[waitingId].src)) { // 如果處理成功，設置訂單為非等待
            activeOrders[waitingId].waiting = false;
        }
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
            driversAtLocation[v].resize(c, (Driver){v, true});
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
