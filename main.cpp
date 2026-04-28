#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <string>
#include <memory>
#include <stdexcept>

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

// Singleton + STL (map, set)
class Bank
{
    map<string, string> users;              // username -> parola
    map<int, unique_ptr<Account>> accounts; // ID -> cont
    set<int> ids;                           // ID-uri unice
    int nextId = 1;

    Bank() {} // constructor privat

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
        users[u] = p;
    }

    void login(const string &u, const string &p)
    {
        if (!users.count(u) || users[u] != p)
            throw BankErr("Date incorecte!");
    }

    int openAcc(int tip)
    {
        unique_ptr<Account> acc = (tip == 1)
                                      ? static_cast<unique_ptr<Account>>(make_unique<Savings>())
                                      : static_cast<unique_ptr<Account>>(make_unique<Checking>());
        int id = nextId++;
        accounts[id] = move(acc);
        ids.insert(id);
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
             << "7. Transfer\n8. Sold\n0. Exit\n> ";
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
                cout << "Cont Economii creat, ID: " << bank.openAcc(1) << "\n";
            }
            else if (opt == 4)
            {
                cout << "Cont Curent creat, ID: " << bank.openAcc(2) << "\n";
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
                int id;
                cout << "ID cont: ";
                cin >> id;
                bank.getAcc(id)->printHistory();
            }
        }
        catch (exception &e)
        {
            cout << "[EROARE] " << e.what() << "\n";
        }
    }

    return 0;
}
