// Email prioritizer for a busy CEO.
// Uses a hand-written, vector-based MaxHeap as the priority queue.

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <stdexcept>

// ---------------------------------------------------------------------------
// Email: one message plus the logic for comparing priority between messages.
// ---------------------------------------------------------------------------
class Email {
public:
    Email() : rank(0), sortableDate(0) {}

    Email(const std::string& category, const std::string& subject, const std::string& date)
        : category(category), subject(subject), date(date),
          rank(categoryRank(category)), sortableDate(toSortableDate(date)) {}

    const std::string& getCategory() const { return category; }
    const std::string& getSubject()  const { return subject; }
    const std::string& getDate()     const { return date; }

    // True if this email should be read before the other one.
    bool hasHigherPriorityThan(const Email& other) const {
        if (rank != other.rank) return rank > other.rank;
        return sortableDate > other.sortableDate;   // newer first on ties
    }

    void display(std::ostream& out) const {
        out << "Next email:\n"
            << "Sender: "  << category << "\n"
            << "Subject: " << subject  << "\n"
            << "Date: "    << date     << "\n";
    }

    static bool isValidCategory(const std::string& c) { return categoryRank(c) > 0; }

private:
    std::string category;
    std::string subject;
    std::string date;
    int rank;          // higher = more important
    long sortableDate; // YYYYMMDD so plain integer comparison orders by date

    static int categoryRank(const std::string& c) {
        if (c == "Boss")            return 5;
        if (c == "Subordinate")     return 4;
        if (c == "Peer")            return 3;
        if (c == "ImportantPerson") return 2;
        if (c == "OtherPerson")     return 1;
        return 0;
    }

    // "MM-DD-YYYY" -> YYYYMMDD
    static long toSortableDate(const std::string& d) {
        if (d.size() != 10 || d[2] != '-' || d[5] != '-')
            throw std::invalid_argument("Bad date format: " + d);
        int month = std::stoi(d.substr(0, 2));
        int day   = std::stoi(d.substr(3, 2));
        int year  = std::stoi(d.substr(6, 4));
        return static_cast<long>(year) * 10000 + month * 100 + day;
    }
};

// ---------------------------------------------------------------------------
// MaxHeap: array (vector) based binary heap of Emails.
// Parent of i is (i-1)/2; children are 2i+1 and 2i+2.
// ---------------------------------------------------------------------------
class MaxHeap {
public:
    bool isEmpty() const { return data.empty(); }
    std::size_t size() const { return data.size(); }

    void insert(const Email& e) {
        data.push_back(e);
        siftUp(data.size() - 1);
    }

    const Email& peekMax() const {
        if (isEmpty()) throw std::out_of_range("peekMax on empty heap");
        return data[0];
    }

    Email removeMax() {
        if (isEmpty()) throw std::out_of_range("removeMax on empty heap");
        Email top = data[0];
        data[0] = data.back();
        data.pop_back();
        if (!data.empty()) siftDown(0);
        return top;
    }

private:
    std::vector<Email> data;

    void swapAt(std::size_t a, std::size_t b) {
        Email tmp = data[a];
        data[a] = data[b];
        data[b] = tmp;
    }

    void siftUp(std::size_t i) {
        while (i > 0) {
            std::size_t parent = (i - 1) / 2;
            if (!data[i].hasHigherPriorityThan(data[parent])) break;
            swapAt(i, parent);
            i = parent;
        }
    }

    void siftDown(std::size_t i) {
        std::size_t n = data.size();
        while (true) {
            std::size_t left = 2 * i + 1;
            std::size_t right = 2 * i + 2;
            std::size_t largest = i;
            if (left < n && data[left].hasHigherPriorityThan(data[largest]))   largest = left;
            if (right < n && data[right].hasHigherPriorityThan(data[largest])) largest = right;
            if (largest == i) break;
            swapAt(i, largest);
            i = largest;
        }
    }
};

// ---------------------------------------------------------------------------
// EmailManager: reads the command file and drives the heap.
// ---------------------------------------------------------------------------
class EmailManager {
public:
    explicit EmailManager(std::ostream& out) : out(out) {}

    void processFile(const std::string& filename) {
        std::ifstream in(filename);
        if (!in) {
            out << "Error: could not open file \"" << filename << "\".\n";
            return;
        }
        std::string line;
        while (std::getline(in, line)) {
            processLine(trim(line));
        }
    }

private:
    MaxHeap queue;
    std::ostream& out;

    void processLine(const std::string& line) {
        if (line.empty()) return;

        if (line.rfind("EMAIL ", 0) == 0)  handleEmail(line.substr(6));
        else if (line == "NEXT")           handleNext();
        else if (line == "READ")           handleRead();
        else if (line == "COUNT")          handleCount();
        else out << "Unknown command: " << line << "\n";
    }

    void handleEmail(const std::string& fields) {
        // Split into exactly three comma-delimited fields.
        std::size_t first = fields.find(',');
        std::size_t last  = fields.rfind(',');
        if (first == std::string::npos || first == last) {
            out << "Malformed EMAIL line: " << fields << "\n";
            return;
        }
        std::string category = trim(fields.substr(0, first));
        std::string subject  = trim(fields.substr(first + 1, last - first - 1));
        std::string date     = trim(fields.substr(last + 1));

        if (!Email::isValidCategory(category)) {
            out << "Unknown sender category: " << category << "\n";
            return;
        }
        try {
            queue.insert(Email(category, subject, date));
        } catch (const std::exception& e) {
            out << "Malformed EMAIL line (" << e.what() << ")\n";
        }
    }

    // NEXT only looks; repeated NEXTs show the same email.
    void handleNext() {
        if (queue.isEmpty()) {
            out << "There are no emails to read.\n";
            return;
        }
        queue.peekMax().display(out);
    }

    // READ removes the top email whether or not it was displayed.
    void handleRead() {
        if (queue.isEmpty()) {
            out << "There are no emails to read.\n";
            return;
        }
        queue.removeMax();
    }

    void handleCount() {
        out << "There are " << queue.size() << " emails to read.\n";
    }

    static std::string trim(const std::string& s) {
        const char* ws = " \t\r\n";
        std::size_t start = s.find_first_not_of(ws);
        if (start == std::string::npos) return "";
        std::size_t end = s.find_last_not_of(ws);
        return s.substr(start, end - start + 1);
    }
};

int main(int argc, char* argv[]) {
    std::string filename;
    if (argc > 1) {
        filename = argv[1];
    } else {
        std::cout << "Enter test file name: ";
        std::getline(std::cin, filename);
    }
    EmailManager manager(std::cout);
    manager.processFile(filename);
    return 0;
}