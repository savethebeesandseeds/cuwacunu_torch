// observation_pipeline.cpp
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"

namespace cuwacunu {
namespace camahjucunu {
namespace BNF {

// Constructor: Register executable functions
observationPipeline::observationPipeline() {
    // Example: Registering a function for "BTCUSDT"
    functions["BTCUSDT"] = [](const ParameterList& params, const FileIDList& fileIDs) {
        std::cout << "Executing BTCUSDT with parameters:\n";
        for (const auto& [key, value] : params) {
            std::cout << "  " << key << " = " << value << "\n";
        }
        std::cout << "File IDs:\n";
        for (const auto& id : fileIDs) {
            std::cout << "  " << id << "\n";
        }
        // Implement actual functionality here, e.g., executing a trade
    };

    // Register more functions as needed
    // functions["ETHUSD"] = [](const ParameterList& params, const FileIDList& fileIDs) { ... };
}

// Helper function to get the current context from the stack
std::string observationPipeline::getCurrentContext() const {
    if (contextStack.empty()) {
        return "";
    }
    std::cout << "Stack contents:\n";
    int level = 0;
    
    std::stack<std::string> aux = contextStack;
    while (!aux.empty()) {
        std::cout << "│ " << std::string(level * 4, ' ') << "└── " << aux.top() << "\n";
        aux.pop();
        ++level;
    }

    return contextStack.top();
}

// Visit NonTerminalNode
void observationPipeline::visit(const NonTerminalNode* node) {
    std::cout << "Visiting NonTerminalNode: " << node->name << "\n";
    // Push the current node's name onto the context stack
    contextStack.push(node->name);

    // Handle NonTerminal
    std::cout << "Handling <" << node->name << "> node.\n";
    for (const auto& child : node->children) {
        child->accept(*this);
    }

    // Pop the current node's name from the context stack after traversal
    contextStack.pop();
}

// Visit IdentifierNode
void observationPipeline::visit(const IdentifierNode* node) {
    std::string currentContext = getCurrentContext();
    std::cout << "Visiting IdentifierNode: " << node->value << " in context: " << currentContext << "\n";

    if (currentContext == "<symbol_spec>") {
        symbol = node->value;
        std::cout << "Symbol set to: " << symbol << "\n";
    }
    else if (currentContext == "<file_ids>") {
        fileIDs.push_back(node->value);
        std::cout << "Added File ID: " << node->value << "\n";
    } else {
        log_secure_fatal("[observationPipeline] Identifier '%s' found in unexpected context '%s'\n", 
            node->value.c_str(), currentContext.c_str());
    }
}

// Handler for <instruction>
void observationPipeline::handleInstruction(const NonTerminalNode* node) {
    std::cout << "Handling <instruction> node.\n";
    for (const auto& child : node->children) {
        child->accept(*this);
    }
    executeFunction();
}

// Handler for <parameter>
void observationPipeline::handleParameter(const NonTerminalNode* node) {
    std::cout << "Handling <parameter> node.\n";
    std::string key, value;
    for (const auto& child : node->children) {
        if (auto idNode = dynamic_cast<const IdentifierNode*>(child.get())) {
            if (key.empty()) {
                key = idNode->value;
                std::cout << "Parameter key extracted: " << key << "\n";
            }
            else {
                value = idNode->value;
                std::cout << "Parameter value extracted: " << value << "\n";
            }
        }
    }
    if (!key.empty() && !value.empty()) {
        parameters[key] = value;
        std::cout << "Parameter added: " << key << " = " << value << "\n";
    }
}

// Helper function to execute the mapped function
void observationPipeline::executeFunction() {
    std::cout << "Executing function for symbol: " << symbol << "\n";
    if (symbol.empty()) {
        std::cerr << "Error: No symbol found in instruction.\n";
        return;
    }

    auto it = functions.find(symbol);
    if (it != functions.end()) {
        it->second(parameters, fileIDs);
    } else {
        std::cerr << "Error: No function registered for symbol: " << symbol << "\n";
    }
}

} // namespace BNF
} // namespace camahjucunu
} // namespace cuwacunu
