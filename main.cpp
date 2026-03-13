#include <iostream>
#include <vector>
#include <string>
#include <string_view>
#include <span>
#include <unordered_map>
#include <algorithm>
#include <ranges>
#include <tuple>
#include <ctime>
#include <cstdio>
#include <limits>
#include <numeric>

// T = {date, status, amount, currency, category, desc, card}
struct Tx {
    std::string date;      // "DD.MM.YYYY"
    std::string status;    // "OK" | "FAILED"
    double amount;
    std::string currency;  // "RUB" | "USD" | "EUR"
    std::string category;
    std::string desc;
    std::string card;      // x[n-4..n-1]
};

// |T| = 18
[[nodiscard]] std::vector<Tx> make_data() {
    return {
        {"01.01.2026","OK",1523.50,"RUB","Продукты","Пятёрочка","7890"},
        {"05.01.2026","OK",320.00,"RUB","Транспорт","Яндекс Такси","7890"},
        {"10.01.2026","OK",4500.00,"RUB","Красота","Барбершоп","1234"},
        {"12.01.2026","OK",890.00,"RUB","Рестораны","KFC","1234"},
        {"15.01.2026","FAILED",100.00,"RUB","Переводы","Перевод Ивану","7890"},
        {"18.01.2026","OK",12750.00,"RUB","Супермаркеты","Лента","5678"},
        {"20.01.2026","OK",250.00,"RUB","Транспорт","Метро","5678"},
        {"22.01.2026","OK",6300.00,"RUB","Рестораны","Якитория","1234"},
        {"25.01.2026","OK",1800.00,"RUB","Продукты","ВкусВилл","7890"},
        {"28.01.2026","OK",3200.00,"RUB","Одежда","H&M","5678"},
        {"01.02.2026","OK",15000.00,"RUB","Техника","DNS","1234"},
        {"05.02.2026","OK",780.00,"RUB","Продукты","Магнит","7890"},
        {"10.02.2026","OK",2100.00,"RUB","Красота","Салон Елена","5678"},
        {"14.02.2026","OK",9500.00,"RUB","Рестораны","Пушкин","1234"},
        {"18.02.2026","OK",430.00,"RUB","Транспорт","Яндекс Такси","7890"},
        {"22.02.2026","OK",5600.00,"RUB","Супермаркеты","Ашан","5678"},
        {"01.03.2026","OK",670.00,"RUB","Продукты","Пятёрочка","7890"},
        {"10.03.2026","OK",1450.00,"RUB","Рестораны","Тануки","1234"},
    };
}

// d(s) -> (y, m, d) in Z^3
// s = "DD.MM.YYYY" -> d = s[0..1], m = s[3..4], y = s[6..9]
[[nodiscard]] constexpr std::tuple<int,int,int> parse_date(std::string_view s) {
    // constexpr digit parse: c - '0'
    auto d2 = [](std::string_view v, int pos) constexpr {
        return (v[pos] - '0') * 10 + (v[pos + 1] - '0');
    };
    auto d4 = [](std::string_view v, int pos) constexpr {
        return (v[pos]-'0')*1000 + (v[pos+1]-'0')*100 + (v[pos+2]-'0')*10 + (v[pos+3]-'0');
    };
    int d = d2(s, 0);
    int m = d2(s, 3);
    int y = d4(s, 6);
    return {y, m, d};
}

// f: (y,m,d) -> Z, monotonic
// f(y,m,d) = 365y + floor(y/4) - floor(y/100) + floor(y/400) + floor((153(m-3)+2)/5) + d
[[nodiscard]] constexpr int to_serial(int y, int m, int d) {
    if (m <= 2) { y--; m += 12; }
    return 365*y + y/4 - y/100 + y/400 + (153*(m-3)+2)/5 + d;
}

// f(d(s))
[[nodiscard]] constexpr int date_serial(std::string_view s) {
    auto [y, m, d] = parse_date(s);
    return to_serial(y, m, d);
}

// g(h) = G[i] where R[i].first <= h < R[i].second, O(1)
// [6,12) -> 0 | [12,18) -> 1 | [18,23) -> 2 | else -> 3
[[nodiscard]] std::string_view greetings() {
    std::time_t now = std::time(nullptr);
    int h = std::localtime(&now)->tm_hour;
    constexpr std::pair<int,int> R[] = {{6,12},{12,18},{18,23}};
    constexpr std::string_view G[] = {
        "Доброе утро", "Добрый день", "Добрый вечер", "Доброй ночи"
    };
    for (int i = 0; i < 3; ++i)
        if (h >= R[i].first && h < R[i].second) return G[i];
    return G[3];
}

// S_card(c) = sum_{i: card(i)=c ^ status(i)="OK"} amount(i)
// cashback(c) = S_card(c) / 100
// T(n) = O(n), S(k) = O(k), k = |{card(i)}|
[[nodiscard]] std::unordered_map<std::string, double> for_each_card(std::span<const Tx> txs) {
    std::unordered_map<std::string, double> agg;
    for (const auto& t : txs)
        if (t.status == "OK") agg[t.card] += t.amount;
    std::cout << "\n  Карта    |   Сумма     | Кешбэк\n";
    std::cout << "  --------+--------------+--------\n";
    for (const auto& [card, total] : agg)
        std::printf("  *%-6s | %10.2f  | %6.2f\n", card.c_str(), total, total/100.0);
    return agg;
}

// TopK(T, k=5): partial_sort -> O(n + k log k)
// R = {t in T | t.status = "OK"}, sort R[0..k) by amount desc
[[nodiscard]] std::vector<Tx> top_five_transaction(std::span<const Tx> txs) {
    std::vector<Tx> ok;
    ok.reserve(txs.size());
    std::ranges::copy_if(txs, std::back_inserter(ok),
        [](const Tx& t) { return t.status == "OK"; });
    int k = std::min(5, static_cast<int>(ok.size()));
    std::ranges::partial_sort(ok, ok.begin() + k,
        [](const Tx& a, const Tx& b) { return a.amount > b.amount; });
    ok.resize(k);
    std::cout << "\n  Top-" << k << ":\n";
    for (int i = 0; i < k; ++i)
        std::printf("  %d. %10.2f RUB | %-15s | %s\n",
            i+1, ok[i].amount, ok[i].category.c_str(), ok[i].desc.c_str());
    return ok;
}

// S_c = sum_{i: cat(i)=c ^ serial(d(i)) in [ref-90, ref] ^ status(i)="OK"} amount(i)
// T(n) = O(n)
[[nodiscard]] std::vector<Tx> spending_by_category(
        std::span<const Tx> txs, std::string_view cat, std::string_view ref_date) {
    int ref = date_serial(ref_date);
    int lo = ref - 90;
    std::vector<Tx> filtered;
    // copy_if: category ^ date in [lo, ref] ^ status = "OK"
    std::ranges::copy_if(txs, std::back_inserter(filtered),
        [&](const Tx& t) {
            if (t.status != "OK" || t.category != cat) return false;
            int d = date_serial(t.date);
            return d >= lo && d <= ref;
        });
    double total = 0.0;
    std::cout << "\n  " << cat << " | [" << ref_date << " - 90d]\n";
    std::cout << "  -------------------------------------------------\n";
    for (const auto& t : filtered) {
        std::printf("  %s | %10.2f | %s\n", t.date.c_str(), t.amount, t.desc.c_str());
        total += t.amount;
    }
    std::printf("  Σ = %.2f RUB\n", total);
    return filtered;
}

// pi(q) = max{k < q : P[0..k-1] = P[q-k..q-1]}
// T(m) = O(m), S(m) = O(m)
[[nodiscard]] std::vector<int> kmp_failure(std::string_view pat) {
    int m = static_cast<int>(pat.size());
    std::vector<int> f(m, 0);
    for (int i = 1, j = 0; i < m; ) {
        if (pat[i] == pat[j]) { f[i++] = ++j; }
        else if (j > 0) { j = f[j-1]; }
        else { f[i++] = 0; }
    }
    return f;
}

// KMP: P subseteq T as substring
// T(n,m) = O(n + m)
[[nodiscard]] bool kmp_search(std::string_view text, std::string_view pat) {
    if (pat.empty()) return true;
    int n = static_cast<int>(text.size());
    int m = static_cast<int>(pat.size());
    if (m > n) return false;
    auto f = kmp_failure(pat);
    for (int i = 0, j = 0; i < n; ) {
        if (text[i] == pat[j]) {
            ++i; ++j;
            if (j == m) return true;
        } else if (j > 0) { j = f[j-1]; }
        else { ++i; }
    }
    return false;
}

// forall c in [A,Z]: c -> c + 32
[[nodiscard]] std::string to_lower_ascii(std::string_view s) {
    std::string r{s};
    for (auto& c : r) if (c >= 'A' && c <= 'Z') c += 32;
    return r;
}

// forall t in T: (P subseteq desc(t)) v (P subseteq cat(t)) -> result
// T(n,m) = O(n * (|desc| + |cat| + m))
[[nodiscard]] std::vector<const Tx*> simple_search(std::span<const Tx> txs, std::string_view query) {
    std::string q = to_lower_ascii(query);
    std::vector<const Tx*> res;
    for (const auto& t : txs) {
        if (kmp_search(to_lower_ascii(t.desc), q) ||
            kmp_search(to_lower_ascii(t.category), q))
            res.push_back(&t);
    }
    std::cout << "\n  \"" << query << "\" (" << res.size() << "):\n";
    for (const auto* t : res)
        std::printf("  %s | %10.2f | %-15s | %s\n",
            t->date.c_str(), t->amount, t->category.c_str(), t->desc.c_str());
    if (res.empty()) std::cout << "  |R| = 0\n";
    return res;
}

int main() {
#ifdef _WIN32
    std::system("chcp 65001 > nul");
#endif
    auto data = make_data();
    int ch;
    do {
        std::cout << "\n===== Финансовая аналитика =====\n";
        std::cout << "1. Топ-5 транзакций глобально\n";
        std::cout << "2. Траты по категории за 90 дней\n";
        std::cout << "3. Информация по картам\n";
        std::cout << "4. Поиск (KMP)\n";
        std::cout << "5. Приветствие\n";
        std::cout << "0. Выход\n";
        std::cout << ">>> ";
        if (!(std::cin >> ch)) break;
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        if (ch == 1) {
            [[maybe_unused]] auto top = top_five_transaction(data);
        } else if (ch == 2) {
            std::cout << "Категория: ";
            std::string cat; std::getline(std::cin, cat);
            std::cout << "Дата (DD.MM.YYYY, Enter=13.03.2026): ";
            std::string dt; std::getline(std::cin, dt);
            if (dt.empty()) dt = "13.03.2026";
            [[maybe_unused]] auto res = spending_by_category(data, cat, dt);
        } else if (ch == 3) {
            [[maybe_unused]] auto agg = for_each_card(data);
        } else if (ch == 4) {
            std::cout << "Запрос: ";
            std::string q; std::getline(std::cin, q);
            [[maybe_unused]] auto res = simple_search(data, q);
        } else if (ch == 5) {
            std::cout << "\n  " << greetings() << "!\n";
        }
    } while (ch != 0);
    return 0;
}
