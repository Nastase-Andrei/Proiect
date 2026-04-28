#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <memory>
#include <stdexcept>
#include <sstream>
#include <ctime>
#include <cstdlib>

using namespace std;

// Formateaza double (100.000000 -> 100)
string formatMoney(double d)
{
    string s = to_string(d);
    s.erase(s.find_last_not_of('0') + 1, string::npos);
    if (!s.empty() && s.back() == '.')
        s.pop_back();
    return s;
}

// Functii helper
vector<string> split(const string &s, char delim)
{
    vector<string> tokens;
    stringstream ss(s);
    string tok;
    while (getline(ss, tok, delim))
        if (!tok.empty())
            tokens.push_back(tok);
    return tokens;
}

string joinIds(const vector<int> &v)
{
    string r;
    for (size_t i = 0; i < v.size(); i++)
        r += (i ? "," : "") + to_string(v[i]);
    return r;
}

vector<int> parseIds(const string &s)
{
    vector<int> r;
    for (auto &tok : split(s, ','))
        r.push_back(stoi(tok));
    return r;
}

// Error Handling
class BankErr : public runtime_error
{
public:
    using runtime_error::runtime_error;
};

// Clasa Account
class Account
{
protected:
    double balance = 0;
    vector<string> history;

public:
    virtual ~Account() = default;
    virtual void withdraw(double a) = 0;
    virtual string type() const = 0;

    void deposit(double a)
    {
        if (a <= 0)
            throw BankErr("Suma invalida!");
        balance += a;
        history.push_back("Depunere: " + formatMoney(a));
    }

    double getBalance() const { return balance; }

    // I/O Streams raport in fisier
    void report(ofstream &f) const
    {
        f << type() << " | Sold: " << formatMoney(balance) << "\nTranzactii:\n";
        for (auto &t : history)
            f << " - " << t << "\n";
    }

    void printHistory() const
    {
        cout << type() << " | Sold: " << formatMoney(balance) << "\nTranzactii:\n";
        for (auto &t : history)
            cout << " - " << t << "\n";
    }
};

class Savings : public Account
{
public:
    void withdraw(double a) override
    {
        if (a > balance)
            throw BankErr("Fonduri insuficiente!");
        balance -= a;
        history.push_back("Retragere Economii: " + formatMoney(a));
    }
    string type() const override { return "Economii"; }
};

class Checking : public Account
{
public:
    void withdraw(double a) override
    {
        if (a > balance + 500)
            throw BankErr("Limita overdraft depasita!");
        balance -= a;
        history.push_back("Retragere Curent: " + formatMoney(a));
    }
    string type() const override { return "Curent"; }
};

// Bank + conturi multiple
class Bank
{
    struct UserData
    {
        string password;
        vector<int> savings;  // ID-uri conturi economii
        vector<int> checking; // ID-uri conturi curente
    };

    map<string, UserData> users;
    map<int, unique_ptr<Account>> accounts;
    set<int> ids;

    Bank()
    {
        srand(time(nullptr));
        ifstream check("users.json");
        if (!check)
        {
            ofstream("users.json") << "{}\n";
        }
        loadUsers();
    }

    // format: "user": "parola|E:id1,id2;C:id3"
    void loadUsers()
    {
        ifstream f("users.json");
        string line;
        while (getline(f, line))
        {
            size_t q[4];
            q[0] = line.find('"');
            for (int i = 1; i < 4; i++)
                q[i] = (q[i - 1] != string::npos) ? line.find('"', q[i - 1] + 1) : string::npos;
            if (q[3] == string::npos)
                continue;

            string user = line.substr(q[0] + 1, q[1] - q[0] - 1);
            string val = line.substr(q[2] + 1, q[3] - q[2] - 1);

            size_t pipe = val.find('|');
            if (pipe == string::npos)
            {
                users[user] = {val, {}, {}};
                continue;
            }

            string pass = val.substr(0, pipe);
            vector<int> sav, chk;
            for (auto &part : split(val.substr(pipe + 1), ';'))
            {
                if (part.size() >= 2 && part[1] == ':')
                {
                    auto idList = parseIds(part.substr(2));
                    (part[0] == 'E' ? sav : chk) = idList;
                }
            }
            users[user] = {pass, sav, chk};
        }
    }

    void saveUsers()
    {
        ofstream f("users.json");
        f << "{\n";
        int i = 0, n = users.size();
        for (auto &[u, d] : users)
        {
            f << "  \"" << u << "\": \"" << d.password
              << "|E:" << joinIds(d.savings)
              << ";C:" << joinIds(d.checking) << "\"";
            if (++i < n)
                f << ",";
            f << "\n";
        }
        f << "}\n";
        f.flush();
    }

    int genId()
    {
        int id = rand() % 9000 + 1000;
        while (ids.count(id))
            id = rand() % 9000 + 1000;
        return id;
    }

public:
    static Bank &get()
    {
        static Bank b;
        return b;
    }

    void reg(const string &u, const string &p)
    {
        if (users.count(u))
            throw BankErr("Utilizator deja existent!");
        users[u] = {p, {}, {}};
        saveUsers();
    }

    void login(const string &u, const string &p)
    {
        if (!users.count(u) || users[u].password != p)
            throw BankErr("Date incorecte!");
    }

    int openAcc(int tip, const string &user)
    {
        if (user.empty() || !users.count(user))
            throw BankErr("Trebuie sa fii autentificat!");

        int id = genId();
        unique_ptr<Account> acc = (tip == 1)
                                      ? static_cast<unique_ptr<Account>>(make_unique<Savings>())
                                      : static_cast<unique_ptr<Account>>(make_unique<Checking>());
        accounts[id] = move(acc);
        ids.insert(id);

        (tip == 1 ? users[user].savings : users[user].checking).push_back(id);
        saveUsers();
        return id;
    }

    Account *getAcc(int id)
    {
        if (!ids.count(id))
            throw BankErr("Cont inexistent!");
        return accounts[id].get();
    }

    void transfer(int from, int to, double amt)
    {
        getAcc(from)->withdraw(amt);
        getAcc(to)->deposit(amt);
    }

    void saveReport(int id)
    {
        ofstream f("raport_" + to_string(id) + ".txt");
        getAcc(id)->report(f);
    }

    string getUserAccounts(const string &user)
    {
        if (!users.count(user))
            throw BankErr("Utilizator inexistent!");
        auto &d = users[user];

        auto listIds = [](const vector<int> &v) -> string
        {
            if (v.empty())
                return "niciun cont";
            string r;
            for (size_t i = 0; i < v.size(); i++)
                r += (i ? ", " : "") + string("ID:") + to_string(v[i]);
            return r;
        };

        return "[Economii] " + listIds(d.savings) +
               " | [Curente] " + listIds(d.checking);
    }
};

// Meniu
int main()
{
    auto &bank = Bank::get();
    string loggedInUser;
    int opt;

    while (true)
    {
        cout << "\n=== SIMULATOR BANCAR ===\n"
             << "1. Register\n2. Login\n3. Open Economii\n"
             << "4. Open Curent\n5. Deposit\n6. Withdraw\n"
             << "7. Transfer\n8. Conturile Mele\n9. Raport\n0. Exit\n> ";
        cin >> opt;

        try
        {
            if (opt == 0)
                break;

            if (opt == 1)
            {
                string u, p;
                cout << "Username: ";
                cin >> u;
                cout << "Parola: ";
                cin >> p;
                bank.reg(u, p);
                loggedInUser = u;
                cout << "Inregistrat si autentificat!\n";
            }
            else if (opt == 2)
            {
                string u, p;
                cout << "Username: ";
                cin >> u;
                cout << "Parola: ";
                cin >> p;
                bank.login(u, p);
                loggedInUser = u;
                cout << "Autentificare reusita!\n";
            }
            else if (opt == 3)
            {
                cout << "Cont Economii creat, ID: " << bank.openAcc(1, loggedInUser) << "\n";
            }
            else if (opt == 4)
            {
                cout << "Cont Curent creat, ID: " << bank.openAcc(2, loggedInUser) << "\n";
            }
            else if (opt == 5)
            {
                int id;
                double s;
                cout << "ID cont: ";
                cin >> id;
                cout << "Suma: ";
                cin >> s;
                bank.getAcc(id)->deposit(s);
                cout << "Sold: " << formatMoney(bank.getAcc(id)->getBalance()) << "\n";
            }
            else if (opt == 6)
            {
                int id;
                double s;
                cout << "ID cont: ";
                cin >> id;
                cout << "Suma: ";
                cin >> s;
                bank.getAcc(id)->withdraw(s);
                cout << "Sold: " << formatMoney(bank.getAcc(id)->getBalance()) << "\n";
            }
            else if (opt == 7)
            {
                int f, t;
                double s;
                cout << "De la ID: ";
                cin >> f;
                cout << "Catre ID: ";
                cin >> t;
                cout << "Suma: ";
                cin >> s;
                bank.transfer(f, t, s);
                cout << "Transfer finalizat!\n";
            }
            else if (opt == 8)
            {
                if (loggedInUser.empty())
                    throw BankErr("Nu esti autentificat!");
                cout << bank.getUserAccounts(loggedInUser) << "\n";
            }
            else if (opt == 9)
            {
                int id;
                cout << "ID cont: ";
                cin >> id;
                bank.saveReport(id);
                cout << "Raport salvat!\n";
            }
        }
        catch (exception &e)
        {
            cout << "[EROARE] " << e.what() << "\n";
        }
    }

    return 0;
}
