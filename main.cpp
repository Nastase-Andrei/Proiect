#include <iostream>
#include <vector>
#include <string>
#include <stdexcept>
#include <memory>

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
    vector<string> history; // STL: vector

public:
    virtual ~Account() = default;
    virtual void withdraw(double a) = 0; // functie pur virtuala
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

// Mostenire: Savings
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

// Mostenire: Checking
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

// Main
int main()
{
    try
    {
        unique_ptr<Account> acc1 = make_unique<Savings>();
        unique_ptr<Account> acc2 = make_unique<Checking>();

        acc1->deposit(1000);
        acc1->withdraw(200);

        acc2->deposit(500);
        acc2->withdraw(800);

        cout << "=== Cont 1 ===\n";
        acc1->printHistory();

        cout << "\n=== Cont 2 ===\n";
        acc2->printHistory();

        // Test eroare
        cout << "\n--- Test eroare ---\n";
        acc1->withdraw(9999); // arunca exceptie
    }
    catch (BankErr &e)
    {
        cout << "[EROARE] " << e.what() << endl;
    }

    return 0;
}
