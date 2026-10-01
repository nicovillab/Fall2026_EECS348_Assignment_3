#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>

// ---------------------------------------------------------------------------
// Email: represents a single email in the CEO's inbox.
// Knows its own sender category, subject line, and date,
// and can compare its priority against another email.
// ---------------------------------------------------------------------------
class Email {
public:
    Email() : category_(""), subject_(""), date_("") {}

    Email(const std::string& category,
          const std::string& subject,
          const std::string& date)
        : category_(category), subject_(subject), date_(date) {}

    // Higher rank = higher priority. Boss first, OtherPerson last.
    static int categoryRank(const std::string& category) {
        static const std::unordered_map<std::string, int> ranks = {
            {"Boss", 5},
            {"Subordinate", 4},
            {"Peer", 3},
            {"ImportantPerson", 2},
            {"OtherPerson", 1}
        };
        auto it = ranks.find(category);
        return (it != ranks.end()) ? it->second : 0;
    }

    // Date is MM-DD-YYYY; convert to a single sortable number YYYYMMDD
    // so newer dates compare as larger.
    static long dateToNumber(const std::string& date) {
        int mm = 0, dd = 0, yyyy = 0;
        char sep1 = 0, sep2 = 0;
        std::istringstream iss(date);
        if (iss >> mm >> sep1 >> dd >> sep2 >> yyyy) {
            return static_cast<long>(yyyy) * 10000L
                 + static_cast<long>(mm) * 100L
                 + static_cast<long>(dd);
        }
        return 0; // unparseable date sorts oldest
    }

    // True if this email has strictly higher priority than `other`:
    // first by sender category, then (on ties) the newest email first.
    bool higherPriorityThan(const Email& other) const {
        int myRank    = categoryRank(category_);
        int otherRank = categoryRank(other.category_);
        if (myRank != otherRank) return myRank > otherRank;
        return dateToNumber(date_) > dateToNumber(other.date_);
    }

    const std::string& category() const { return category_; }
    const std::string& subject()  const { return subject_; }
    const std::string& date()     const { return date_; }

private:
    std::string category_;
    std::string subject_;
    std::string date_;
};

// ---------------------------------------------------------------------------
// MaxHeap: array-based (vector) max-heap of Emails.
// Implemented from scratch (no std::priority_queue, no make_heap).
// The "largest" element is the highest-priority email.
// ---------------------------------------------------------------------------
class MaxHeap {
public:
    MaxHeap() : heap_() {}

    bool empty() const { return heap_.empty(); }
    int  size()  const { return static_cast<int>(heap_.size()); }

    // Insert a new email and restore the heap property upward.
    void push(const Email& email) {
        heap_.push_back(email);
        siftUp(static_cast<int>(heap_.size()) - 1);
    }

    // Return (without removing) the highest-priority email.
    const Email& top() const {
        return heap_.front();
    }

    // Remove the highest-priority email and restore the heap property downward.
    void pop() {
        if (heap_.empty()) return;
        heap_.front() = heap_.back();
        heap_.pop_back();
        if (!heap_.empty()) siftDown(0);
    }

private:
    std::vector<Email> heap_;

    int parent(int i) const { return (i - 1) / 2; }
    int leftChild(int i) const { return 2 * i + 1; }
    int rightChild(int i) const { return 2 * i + 2; }

    void siftUp(int i) {
        while (i > 0 && heap_[i].higherPriorityThan(heap_[parent(i)])) {
            std::swap(heap_[i], heap_[parent(i)]);
            i = parent(i);
        }
    }

    void siftDown(int i) {
        int n = static_cast<int>(heap_.size());
        while (true) {
            int largest = i;
            int left  = leftChild(i);
            int right = rightChild(i);
            if (left  < n && heap_[left].higherPriorityThan(heap_[largest]))
                largest = left;
            if (right < n && heap_[right].higherPriorityThan(heap_[largest]))
                largest = right;
            if (largest == i) break;
            std::swap(heap_[i], heap_[largest]);
            i = largest;
        }
    }
};

// ---------------------------------------------------------------------------
// EmailPrioritizer: the CEO's email manager.
// Owns the MaxHeap and processes all commands from the test file.
// ---------------------------------------------------------------------------
class EmailPrioritizer {
public:
    explicit EmailPrioritizer(std::ostream& out = std::cout) : heap_(), out_(out) {}

    void addEmail(const std::string& category,
                  const std::string& subject,
                  const std::string& date) {
        heap_.push(Email(category, subject, date));
    }

    void nextEmail() {
        if (heap_.empty()) {
            out_ << "No emails to read.\n";
            return;
        }
        const Email& email = heap_.top();
        out_ << "Next email:\n"
             << "Sender: " << email.category() << "\n"
             << "Subject: " << email.subject() << "\n"
             << "Date: " << email.date() << "\n";
    }

    void readEmail() {
        if (heap_.empty()) {
            out_ << "No emails to read.\n";
            return;
        }
        heap_.pop();
    }

    void countEmails() const {
        out_ << "There are " << heap_.size() << " emails to read.\n";
    }

    // Read the whole test file and execute each command.
    // Returns false if the file cannot be opened.
    bool processFile(const std::string& filename) {
        std::ifstream in(filename);
        if (!in.is_open()) return false;

        std::string line;
        while (std::getline(in, line)) {
            // Trim trailing carriage returns (Windows line endings).
            if (!line.empty() && line.back() == '\r') line.pop_back();

            if (line.rfind("EMAIL ", 0) == 0) {
                std::string body = line.substr(6);
                std::string category, subject, date;
                if (parseEmail(body, category, subject, date)) {
                    addEmail(category, subject, date);
                }
            } else if (line == "NEXT") {
                nextEmail();
            } else if (line == "READ") {
                readEmail();
            } else if (line == "COUNT") {
                countEmails();
            }
            // Blank or unknown lines are ignored.
        }
        return true;
    }

private:
    MaxHeap heap_;
    std::ostream& out_;

    // Split "Category,Subject,Date" into its three comma-delimited fields.
    // The subject may contain spaces but never commas.
    static bool parseEmail(const std::string& body,
                           std::string& category,
                           std::string& subject,
                           std::string& date) {
        std::string::size_type first = body.find(',');
        if (first == std::string::npos) return false;
        std::string::size_type second = body.find(',', first + 1);
        if (second == std::string::npos) return false;

        category = body.substr(0, first);
        subject  = body.substr(first + 1, second - first - 1);
        date     = body.substr(second + 1);
        return !category.empty() && !date.empty();
    }
};

// ---------------------------------------------------------------------------
// main: run the CEO email prioritizer on a test file.
// Usage: ./ceo_emails <test-file>   (defaults to "test.txt" if omitted)
// ---------------------------------------------------------------------------
int main(int argc, char* argv[]) {
    std::string filename = (argc > 1) ? argv[1] : "test.txt";

    EmailPrioritizer prioritizer;
    if (!prioritizer.processFile(filename)) {
        std::cerr << "Error: could not open file \"" << filename << "\".\n";
        return 1;
    }
    return 0;
}