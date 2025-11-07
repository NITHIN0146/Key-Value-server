#include "httplib.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    httplib::Client cli("http://127.0.0.1:8080");

    while (true) {
        cout << "\n1. Create/Update\n2. Read\n3. Delete\n4. Exit\nChoose one option from above: ";
        int choice; cin >> choice;

        int key;
        string value;
        cin.ignore();
        if (choice == 1) {
            cout << "Key : ";
            cin >> key;
            cin.ignore();
            cout << "Value : ";
            getline(cin, value);

            auto res = cli.Post(("/Create?id=" + to_string(key)).c_str(), value, "text/plain");
            if (res) cout << res->body;
            else cout << "Could not reach server\n";
        }

        else if (choice == 2) {
            cout << "Key: ";
            cin >> key;

            auto res = cli.Get(("/Read?id=" + to_string(key)).c_str());
            if (res) cout << "Response: " << res->body << endl;
            else cout << "Could not reach server\n";
        }

        else if (choice == 3) {
            cout << "Key: ";
            cin >> key;

            auto res = cli.Delete(("/Delete?id=" + to_string(key)).c_str());
            if (res) cout << res->body;
            else cout << "Could not reach server\n";
        }
        else if (choice == 4) break;
        else {
            cout << "Please choose from given options\n";
        }
    }

    return 0;
}