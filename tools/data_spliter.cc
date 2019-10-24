#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

int main(int argc, char* argv[]) {
  if (argc < 2) {
    cout << "Args: file_path" << endl;
    return 0;
  }

  auto file = argv[1];
  ifstream ifs(file);
  string line;
  vector<vector<string>> table;
  while (getline(ifs, line)) {
    stringstream ss(line);
    string data;
    vector<string> sample;
    while (getline(ss, data, ',')) {
      if (data == "0") data = "0.00001"; // temporarily fix zero value issue
      sample.push_back(data);
    }
    table.push_back(sample);
  }
  int n_samples = table.size();
  int n_attributes = table[0].size();
  cout << "number of samples: " << n_samples << endl
       << "number of attributes (including label): " << n_attributes << endl;

  string f1 = "client_0.txt", f2 = "client_1.txt", f3 = "client_2.txt";
  ofstream out1(f1), out2(f2), out3(f3);
  int n_attr_per_client = n_attributes / 3;
  for (auto entry : table) {
    out1 << "1.0,"; // x0
    for (int i = 0; i < n_attributes; ++i) {
      if (i < n_attr_per_client) {
        out3 << entry[i];
        if (i == n_attr_per_client - 1) {
          out3 << endl;
        } else {
          out3 << ",";
        }
      } else if (i < n_attr_per_client * 2) {
        out2 << entry[i];
        if (i == n_attr_per_client * 2 - 1) {
          out2 << endl;
        } else {
          out2 << ",";
        }
      } else {
        if (i == n_attributes - 1) { // deal with label
          if (entry[i] == "g") {
            out1 << "1" << endl;
          } else if (entry[i] == "b") {
            out1 << "0" << endl;
          }
        } else {
          out1 << entry[i] << ",";
        }
      }
    }
  }
  return 0;
}