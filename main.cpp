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

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

using namespace std;
using namespace ftxui;

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

// Clase, Polimorfism, Mostenire, Functii Virtuale
class Account
{
protected:
    double balance = 0;
    vector<string> history;

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

    // I/O Streams export raport in fisier
    void report(ofstream &f) const
    {
        f << type() << " | Sold: " << formatMoney(balance) << "\nTranzactii:\n";
        for (auto &t : history)
            f << " - " << t << "\n";
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

// Singleton + map, set
class Bank
{
    struct UserData
    {
        string password;
        vector<int> savings;  // ID-uri conturi economii
        vector<int> checking; // ID-uri conturi curente
    };

    map<string, UserData> users;            // map (autentificare)
    map<int, unique_ptr<Account>> accounts; // map (conturi)
    set<int> ids;                           // set (ID-uri unice)

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

    // format: "username": "parola|E:id1,id2;C:id3,id4"

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
    } // Singleton

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

// Interfata FTXUI

int main()
{
    auto &bank = Bank::get();
    auto screen = ScreenInteractive::Fullscreen();

    int page = 0; // 0 = meniu, 1 = formular
    string mesaj = "Bine ai venit!";
    string username, password, id_str, id2_str, suma_str;
    string loggedInUser;

    // Componente input
    auto input_user = Input(&username, "username");
    auto input_pass = Input(&password, "parola");
    auto input_id = Input(&id_str, "id cont");
    auto input_id2 = Input(&id2_str, "id destinatie");
    auto input_suma = Input(&suma_str, "suma");

    // Enum pentru operatii
    enum Op
    {
        REG,
        LOGIN,
        OPEN_SAV,
        OPEN_CHK,
        DEP,
        WDR,
        XFER,
        MY_ACC,
        REPORT,
        EXIT
    };

    // Meniu
    vector<string> entries = {
        "Register", "Login", "Open Account (Economii)",
        "Open Account (Curent)", "Deposit", "Withdraw",
        "Transfer", "Conturile Mele", "Raport", "Exit"};
    int selected = 0;

    MenuOption menu_opt;
    menu_opt.entries_option.transform = [](const EntryState &s)
    {
        string prefix = s.focused ? "> " : "  ";
        auto el = text(prefix + s.label) | color(Color::White);
        if (s.focused)
            el = el | inverted;
        return el;
    };
    auto menu = Menu(&entries, &selected, menu_opt);

    ButtonOption btn_style;
    btn_style.transform = [](const EntryState &s)
    {
        auto el = text(s.label) | center | color(Color::White);
        if (s.focused)
            el = el | inverted;
        return el | border;
    };

    auto clearInputs = [&]
    {
        username.clear();
        password.clear();
        id_str.clear();
        id2_str.clear();
        suma_str.clear();
    };

    // Buton Executa
    auto btn_ok = Button("  Executa  ", [&]
                         {
        try {
            int id    = id_str.empty()   ? 0 : stoi(id_str);
            int id2   = id2_str.empty()  ? 0 : stoi(id2_str);
            double s  = suma_str.empty() ? 0 : stod(suma_str);

            switch (selected) {
            case REG:
                bank.reg(username, password);
                loggedInUser = username;
                mesaj = "Inregistrat cu succes! (autentificat automat)";
                break;
            case LOGIN:
                bank.login(username, password);
                loggedInUser = username;
                mesaj = "Autentificare reusita!";
                break;
            case OPEN_SAV:
                mesaj = "Cont Economii creat, ID: " + to_string(bank.openAcc(1, loggedInUser));
                break;
            case OPEN_CHK:
                mesaj = "Cont Curent creat, ID: " + to_string(bank.openAcc(2, loggedInUser));
                break;
            case DEP:
                bank.getAcc(id)->deposit(s);
                mesaj = "Depunere OK! Sold: " + formatMoney(bank.getAcc(id)->getBalance());
                break;
            case WDR:
                bank.getAcc(id)->withdraw(s);
                mesaj = "Retragere OK! Sold: " + formatMoney(bank.getAcc(id)->getBalance());
                break;
            case XFER:
                bank.transfer(id, id2, s);
                mesaj = "Transfer finalizat!";
                break;
            case MY_ACC:
                if (loggedInUser.empty()) throw BankErr("Trebuie sa fii autentificat!");
                mesaj = bank.getUserAccounts(loggedInUser);
                break;
            case REPORT:
                bank.saveReport(id);
                mesaj = "Raport salvat in raport_" + id_str + ".txt";
                break;
            case EXIT:
                screen.Exit(); return;
            }
        } catch (exception& e) {
            mesaj = string("[EROARE] ") + e.what();
        }
        clearInputs();
        page = 0; }, btn_style);

    auto btn_back = Button("  Inapoi  ", [&]
                           {
        clearInputs();
        page = 0; }, btn_style);

    // Layout
    auto page_menu = Container::Vertical({menu});
    auto page_form = Container::Vertical({input_user, input_pass, input_id, input_id2, input_suma,
                                          Container::Horizontal({btn_ok, btn_back})});
    auto tab = Container::Tab({page_menu, page_form}, &page);

    tab |= CatchEvent([&](Event e)
                      {
        if (page == 0 && e == Event::Return) {
            selected == EXIT ? screen.Exit() : (void)(page = 1);
            return true;
        }
        return false; });

    // Renderer
    auto renderer = Renderer(tab, [&]
                             {
        Element content;

        if (page == 0) {
            content = vbox({
                text("SIMULATOR SISTEM BANCAR") | bold | center,
                separator(),
                menu->Render() | vscroll_indicator | frame,
                separator(),
                text("Foloseste sagetile + ENTER") | dim | center,
            });
        } else {
            Elements form;
            form.push_back(text("  " + entries[selected] + "  ") | bold | center);
            form.push_back(separator());

            bool needUser = (selected <= LOGIN);
            bool needId   = (selected >= DEP && selected <= XFER) || selected == REPORT;
            bool needId2  = (selected == XFER);
            bool needSuma = (selected >= DEP && selected <= XFER);

            if (needUser) { form.push_back(input_user->Render()); form.push_back(input_pass->Render()); }
            if (needId)   form.push_back(input_id->Render());
            if (needId2)  form.push_back(input_id2->Render());
            if (needSuma) form.push_back(input_suma->Render());

            form.push_back(separator());
            form.push_back(hbox({btn_ok->Render(), text(" "), btn_back->Render()}) | center);

            content = vbox(form);
        }

        auto statusColor = mesaj.find("EROARE") != string::npos ? color(Color::Red) : color(Color::Green);
        return vbox({content | border, text(" Status: " + mesaj) | statusColor}); });

    screen.Loop(renderer);
    return 0;
}
