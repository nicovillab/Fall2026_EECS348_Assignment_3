/*
 * ============================================================================
 * Program:        EECS 348 Assignment 3 - CEO Email Prioritizer (GenAI version)
 * File:           EmailPrioritizer.cpp
 * Author:         Nicolas Villalobos
 * Creation date:  10-01-2026
 * Revision date:  10-01-2026
 *
 * Description:    C++ program that prioritizes emails for a busy company CEO.
 *                 Emails are stored in a priority queue implemented as a
 *                 list-based (std::vector) MaxHeap written from scratch.
 *                 Priority is by sender category (Boss > Subordinate > Peer >
 *                 ImportantPerson > OtherPerson); within a category the newest
 *                 date is read first; if the date also ties, the email that
 *                 appears later in the file is treated as newer.
 *
 * Inputs:         A test file of commands, given as the first command-line
 *                 argument (./EmailPrioritizer test.txt). If no argument is
 *                 given, the file name is read from standard input.
 *                 Commands: EMAIL <category>,<subject>,<date MM-DD-YYYY>
 *                           NEXT, READ, COUNT
 *
 * Output:         Terminal output (stdout) for NEXT and COUNT in the format
 *                 shown in the assignment. Errors and the file-name prompt go
 *                 to stderr so they never mix with graded output.
 *
 * Collaborators + other sources:
 *                 - No human collaborators.
 *                 - Claude (Anthropic), https://claude.ai, new browser chat:
 *                   generated the base code from the prompt recorded in the
 *                   GenAI analysis PDF.
 *                 - Claude (Anthropic) assistant, https://claude.ai: helped
 *                   identify and implement the revisions listed below and
 *                   draft comments.
 *                 - Mistral Le Chat, https://chat.mistral.ai: generated a
 *                   second program that was analyzed; none of its code is used.
 *
 * Revisions:
 *   1. Output matches the assignment's display format (blank line between
 *      outputs, indented Sender/Subject/Date lines).
 *   2. File-name prompt and error messages moved to stderr; program returns a
 *      nonzero exit code when the file cannot be opened.
 *   3. READ on an empty inbox is silent (READ never prints output).
 *   4. Deterministic tie-break using an insertion sequence number when two
 *      emails share both category and date.
 *   5. Date validation now checks digits and month/day ranges.
 *   6. Heap swap uses std::swap (moves strings) instead of three full copies;
 *      removeMax no longer returns an unused copy of the email.
 *   7. Full prologue, block, and line comments added.
 * ============================================================================
 */

#include <iostream>   // std::cout, std::cerr, std::cin
#include <fstream>    // std::ifstream for reading the test file
#include <string>     // std::string
#include <vector>     // std::vector: the list that stores the heap
#include <stdexcept>  // std::invalid_argument, std::out_of_range
#include <utility>    // std::swap
#include <cctype>     // std::isdigit
#include <cstddef>    // std::size_t

/* ----------------------------------------------------------------------------
 * Class Email
 * Holds one email plus the precomputed numbers used to compare priority, so
 * comparisons inside the heap are plain integer comparisons.
 * ------------------------------------------------------------------------- */
class Email {                                                    // one email in the CEO's inbox
public:                                                          // members usable by other classes
    Email(const std::string& category, const std::string& subject, // constructor: category and subject text
          const std::string& date, std::size_t sequence)         // date text and file position
        : category(category),                                    // store sender category text
          subject(subject),                                      // store subject line text
          date(date),                                            // store original date text for display
          rank(categoryRank(category)),                          // convert category to a number once
          sortableDate(toSortableDate(date)),                    // convert MM-DD-YYYY to YYYYMMDD once
          sequence(sequence) {}                                  // position of this email in the file

    bool hasHigherPriorityThan(const Email& other) const {       // true if this email is read before 'other'
        if (rank != other.rank) return rank > other.rank;        // 1) higher category wins
        if (sortableDate != other.sortableDate)                  // categories tie, so compare dates
            return sortableDate > other.sortableDate;            // 2) newer date wins
        return sequence > other.sequence;                        // 3) later in file wins
    }                                                            // end hasHigherPriorityThan

    void display(std::ostream& out) const {                      // print this email in NEXT format
        out << "Next email:\n";                                  // header line
        out << "\tSender: "  << category << "\n";                // indented sender line
        out << "\tSubject: " << subject  << "\n";                // indented subject line
        out << "\tDate: "    << date     << "\n";                // indented date line
    }                                                            // end display

    static bool isValidCategory(const std::string& c) {          // true if c is one of the five categories
        return categoryRank(c) > 0;                              // unknown categories have rank 0
    }                                                            // end isValidCategory

private:                                                         // members only this class can use
    std::string category;                                        // sender category exactly as given
    std::string subject;                                         // subject line (spaces allowed, no commas)
    std::string date;                                            // date exactly as given, MM-DD-YYYY
    int rank;                                                    // 5 = Boss ... 1 = OtherPerson
    long sortableDate;                                           // YYYYMMDD, larger number = newer date
    std::size_t sequence;                                        // order read from the file (0, 1, 2, ...)

    static int categoryRank(const std::string& c) {              // map category name to priority number
        if (c == "Boss")            return 5;                    // read first
        if (c == "Subordinate")     return 4;                    // read second
        if (c == "Peer")            return 3;                    // read third
        if (c == "ImportantPerson") return 2;                    // read fourth
        if (c == "OtherPerson")     return 1;                    // read last
        return 0;                                                // not a valid category
    }                                                            // end categoryRank

    static long toSortableDate(const std::string& d) {           // "MM-DD-YYYY" -> YYYYMMDD, throws if bad
        if (d.size() != 10 || d[2] != '-' || d[5] != '-')        // check length and dash positions
            throw std::invalid_argument("bad date format: " + d); // reject wrong shape
        for (std::size_t i = 0; i < d.size(); ++i) {             // check every character
            if (i == 2 || i == 5) continue;                      // skip the two dashes
            if (!std::isdigit(static_cast<unsigned char>(d[i]))) // every other character must be a digit
                throw std::invalid_argument("bad date format: " + d); // reject non-digits
        }                                                        // end digit check loop
        int month = std::stoi(d.substr(0, 2));                   // characters 0-1 are the month
        int day   = std::stoi(d.substr(3, 2));                   // characters 3-4 are the day
        int year  = std::stoi(d.substr(6, 4));                   // characters 6-9 are the year
        if (month < 1 || month > 12 || day < 1 || day > 31)      // basic range check
            throw std::invalid_argument("bad date value: " + d); // reject impossible dates
        return static_cast<long>(year) * 10000L + month * 100L + day; // combine into YYYYMMDD
    }                                                            // end toSortableDate
};                                                               // end class Email

/* ----------------------------------------------------------------------------
 * Class MaxHeap
 * List-based binary max-heap built from scratch on a std::vector.
 * For index i: parent = (i-1)/2, left child = 2i+1, right child = 2i+2.
 * No std::priority_queue or std::make_heap / push_heap / pop_heap is used.
 * ------------------------------------------------------------------------- */
class MaxHeap {                                                  // priority queue of emails
public:                                                          // members usable by other classes
    bool isEmpty() const { return data.empty(); }                // true when no emails are stored
    std::size_t size() const { return data.size(); }             // number of emails stored

    void insert(const Email& e) {                                // add an email, O(log n)
        data.push_back(e);                                       // place it in the next free slot
        siftUp(data.size() - 1);                                 // move it up to its correct position
    }                                                            // end insert

    const Email& peekMax() const {                               // look at top email without removing, O(1)
        if (isEmpty()) throw std::out_of_range("peekMax on empty heap"); // guard against empty heap
        return data[0];                                          // root is the highest priority
    }                                                            // end peekMax

    void removeMax() {                                           // remove top email, O(log n)
        if (isEmpty()) throw std::out_of_range("removeMax on empty heap"); // guard against empty heap
        std::swap(data[0], data.back());                         // move last email to the root (no copy)
        data.pop_back();                                         // drop the old root, now at the end
        if (!data.empty()) siftDown(0);                          // restore heap order from the root down
    }                                                            // end removeMax

private:                                                         // members only this class can use
    std::vector<Email> data;                                     // the list that stores the heap

    void siftUp(std::size_t i) {                                 // move element i up while it beats its parent
        while (i > 0) {                                          // stop at the root
            std::size_t parent = (i - 1) / 2;                    // parent index
            if (!data[i].hasHigherPriorityThan(data[parent])) break; // already in order, stop
            std::swap(data[i], data[parent]);                    // move child above parent
            i = parent;                                          // continue from the parent's slot
        }                                                        // end while
    }                                                            // end siftUp

    void siftDown(std::size_t i) {                               // move element i down while a child beats it
        std::size_t n = data.size();                             // current heap size
        while (true) {                                           // loop until heap order is restored
            std::size_t left = 2 * i + 1;                        // left child index
            std::size_t right = 2 * i + 2;                       // right child index
            std::size_t largest = i;                             // assume current node is largest
            if (left < n && data[left].hasHigherPriorityThan(data[largest]))   largest = left;  // left is bigger
            if (right < n && data[right].hasHigherPriorityThan(data[largest])) largest = right; // right is bigger
            if (largest == i) break;                             // no child is bigger, done
            std::swap(data[i], data[largest]);                   // move the bigger child up
            i = largest;                                         // continue from that child's slot
        }                                                        // end while
    }                                                            // end siftDown
};                                                               // end class MaxHeap

/* ----------------------------------------------------------------------------
 * Class EmailManager
 * Reads the command file line by line and drives the MaxHeap.
 * ------------------------------------------------------------------------- */
class EmailManager {                                             // runs the commands from the test file
public:                                                          // members usable by main
    EmailManager(std::ostream& out, std::ostream& err)           // out = normal output, err = errors
        : out(out), err(err), nextSequence(0), printedAnything(false) {} // start with nothing printed

    bool processFile(const std::string& filename) {              // run every command; false if not opened
        std::ifstream in(filename);                              // open the test file
        if (!in) {                                               // could not open it
            err << "Error: could not open file \"" << filename << "\".\n"; // report on stderr
            return false;                                        // tell main it failed
        }                                                        // end open check
        std::string line;                                        // holds one line of the file
        while (std::getline(in, line)) {                         // read until end of file
            processLine(trim(line));                             // handle the trimmed line
        }                                                        // end while
        return true;                                             // whole file processed
    }                                                            // end processFile

private:                                                         // members only this class can use
    MaxHeap queue;                                               // priority queue of unread emails
    std::ostream& out;                                           // normal output (stdout)
    std::ostream& err;                                           // error output (stderr)
    std::size_t nextSequence;                                    // sequence number for the next EMAIL
    bool printedAnything;                                        // true once any block has been printed

    void startBlock() {                                          // blank line before every block but the first
        if (printedAnything) out << "\n";                        // separator between blocks
        printedAnything = true;                                  // later blocks get a separator
    }                                                            // end startBlock

    void processLine(const std::string& line) {                  // decide which command a line is
        if (line.empty()) return;                                // ignore blank lines
        if (line.rfind("EMAIL ", 0) == 0) handleEmail(line.substr(6)); // starts with "EMAIL "
        else if (line == "NEXT")          handleNext();          // show top email
        else if (line == "READ")          handleRead();          // remove top email
        else if (line == "COUNT")         handleCount();         // show unread count
        else err << "Unknown command: " << line << "\n";         // report a bad command
    }                                                            // end processLine

    void handleEmail(const std::string& fields) {                // parse "<category>,<subject>,<date>"
        std::size_t first = fields.find(',');                    // comma after the category
        std::size_t last  = fields.rfind(',');                   // comma before the date
        if (first == std::string::npos || first == last) {       // need two different commas
            err << "Malformed EMAIL line: " << fields << "\n";   // report on stderr
            return;                                              // skip this line
        }                                                        // end comma check
        std::string category = trim(fields.substr(0, first));    // text before the first comma
        std::string subject  = trim(fields.substr(first + 1, last - first - 1)); // text between commas
        std::string date     = trim(fields.substr(last + 1));    // text after the last comma

        if (!Email::isValidCategory(category)) {                 // reject unknown senders
            err << "Unknown sender category: " << category << "\n"; // report on stderr
            return;                                              // skip this line
        }                                                        // end category check
        try {                                                    // Email constructor may throw on bad date
            queue.insert(Email(category, subject, date, nextSequence)); // build and add the email
            ++nextSequence;                                      // next email gets a later number
        } catch (const std::exception& e) {                      // bad date format or value
            err << "Malformed EMAIL line (" << e.what() << ")\n"; // report on stderr
        }                                                        // end try/catch
    }                                                            // end handleEmail

    void handleNext() {                                          // NEXT: show top email, do not remove
        startBlock();                                            // blank line between outputs
        if (queue.isEmpty()) {                                   // nothing to show
            out << "There are no emails to read.\n";             // tell the CEO the inbox is empty
            return;                                              // done
        }                                                        // end empty check
        queue.peekMax().display(out);                            // print the top email
    }                                                            // end handleNext

    void handleRead() {                                          // READ: remove top email, never prints
        if (queue.isEmpty()) return;                             // nothing to remove
        queue.removeMax();                                       // CEO has dealt with the top email
    }                                                            // end handleRead

    void handleCount() {                                         // COUNT: show number of unread emails
        startBlock();                                            // blank line between outputs
        out << "There are " << queue.size() << " emails to read.\n"; // print the count
    }                                                            // end handleCount

    static std::string trim(const std::string& s) {              // remove whitespace from both ends
        const char* ws = " \t\r\n";                              // characters treated as whitespace
        std::size_t start = s.find_first_not_of(ws);             // first real character
        if (start == std::string::npos) return "";               // string was all whitespace
        std::size_t end = s.find_last_not_of(ws);                // last real character
        return s.substr(start, end - start + 1);                 // the trimmed middle part
    }                                                            // end trim
};                                                               // end class EmailManager

/* ----------------------------------------------------------------------------
 * main
 * ------------------------------------------------------------------------- */
int main(int argc, char* argv[]) {                               // program entry point
    std::string filename;                                        // name of the test file
    if (argc > 1) {                                              // file given on the command line
        filename = argv[1];                                      // use the first argument
    } else {                                                     // no argument given
        std::cerr << "Enter test file name: ";                   // prompt on stderr, not stdout
        std::getline(std::cin, filename);                        // read the name from the keyboard
    }                                                            // end argument check
    EmailManager manager(std::cout, std::cerr);                  // output to stdout, errors to stderr
    return manager.processFile(filename) ? 0 : 1;                // 0 = success, 1 = file not opened
}                                                                // end main
