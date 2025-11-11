#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <sstream>
#include <algorithm>
#include <limits>
#include <tuple>
#include <fstream> // Necessário para gerar o arquivo DOT

using namespace std;

enum EstadoDFS { NAO_VISITADO, VISITADO, COMPLETO };

// Estrutura para representar cada Atividade (Vértice do Grafo)
struct Atividade {
    string id;              
    int duracao;            

    // Tempos de Início e Fim (Cedo e Tarde)
    int ES = 0;             
    int EF = 0;             
    int LS = numeric_limits<int>::max(); 
    int LF = numeric_limits<int>::max(); 

    int folga = 0;

    // Relações
    vector<string> precedentes; 
    vector<string> sucessores;  
    
    EstadoDFS estado = NAO_VISITADO;
};

void construirGrafo(map<string, Atividade>& atividades, const vector<tuple<string, int, string>>& dados) {
    for (const auto& tupla : dados) {
        string id = get<0>(tupla);
        int duracao = get<1>(tupla);
        string precs_str = get<2>(tupla);

        atividades[id] = {id, duracao};
        stringstream ss(precs_str);
        string prec_id;

        while (getline(ss, prec_id, ',')) {
            if (!prec_id.empty() && prec_id != "-") {
                atividades[id].precedentes.push_back(prec_id);
            }
        }
    }

    for (auto const& pair : atividades) {
        const string& id = pair.first;
        for (const string& prec_id : pair.second.precedentes) {
            if (atividades.count(prec_id)) {
                atividades[prec_id].sucessores.push_back(id);
            }
        }
    }
}

// DFS para Ordenação e Detecção de Ciclos
bool dfsTopologicalSort(const string& id, map<string, Atividade>& atividades, vector<string>& ordem_topologica) {
    Atividade& ativ = atividades.at(id);
    ativ.estado = VISITADO; 

    for (const string& sucessor_id : ativ.sucessores) {
        if (atividades.count(sucessor_id)) {
            Atividade& sucessor = atividades.at(sucessor_id);
            
            if (sucessor.estado == VISITADO) {
                cerr << "ERRO: Ciclo detectado no grafo: " << id << " -> " << sucessor_id << endl;
                return false; 
            }
            
            if (sucessor.estado == NAO_VISITADO) {
                if (!dfsTopologicalSort(sucessor_id, atividades, ordem_topologica)) {
                    return false; 
                }
            }
        }
    }

    ativ.estado = COMPLETO; 
    ordem_topologica.push_back(id); 
    return true; 
}

// Função Principal de Ordenação
bool realizarOrdenacaoTopologica(map<string, Atividade>& atividades, vector<string>& ordem_topologica) {
    ordem_topologica.clear();
    
    for (auto& pair : atividades) {
        pair.second.estado = NAO_VISITADO;
    }
    
    for (auto& pair : atividades) {
        if (pair.second.estado == NAO_VISITADO) {
            if (!dfsTopologicalSort(pair.first, atividades, ordem_topologica)) {
                return false; 
            }
        }
    }
    
    reverse(ordem_topologica.begin(), ordem_topologica.end());
    return true;
}

void forwardPass(map<string, Atividade>& atividades, const vector<string>& ordem_topologica) {
    for (const string& id : ordem_topologica) {
        Atividade& ativ = atividades.at(id);
        
        if (ativ.precedentes.empty()) {
            ativ.ES = 0;
        } else {
            int max_ef = 0;
            for (const string& prec_id : ativ.precedentes) {
                if (atividades.count(prec_id)) {
                    max_ef = max(max_ef, atividades.at(prec_id).EF);
                }
            }
            ativ.ES = max_ef;
        }
        ativ.EF = ativ.ES + ativ.duracao;
    }
}

void backwardPass(map<string, Atividade>& atividades, const vector<string>& ordem_topologica, int duracao_projeto) {
    for (auto it = ordem_topologica.rbegin(); it != ordem_topologica.rend(); ++it) {
        const string& id = *it;
        Atividade& ativ = atividades.at(id);
        
        if (ativ.sucessores.empty()) {
            ativ.LF = duracao_projeto;
        } else {
            int min_ls = numeric_limits<int>::max();
            for (const string& suc_id : ativ.sucessores) {
                if (atividades.count(suc_id)) {
                    min_ls = min(min_ls, atividades.at(suc_id).LS);
                }
            }
            ativ.LF = min_ls;
        }
        
        ativ.LS = ativ.LF - ativ.duracao;
        ativ.folga = ativ.LS - ativ.ES;
    }
}

vector<string> calcularPERT_CPM(map<string, Atividade>& atividades, bool& sucesso) {
    vector<string> ordem_topologica;
    sucesso = true; 

    if (!realizarOrdenacaoTopologica(atividades, ordem_topologica)) {
        cout << "\nERRO: O cálculo PERT/CPM não pode ser completado devido a um ciclo no projeto." << endl;
        sucesso = false; 
        return ordem_topologica; 
    }
    cout << "Ordenação Topológica (DFS) completa. Ordem: ";
    for (const string& id : ordem_topologica) {
        cout << id << " ";
    }
    cout << endl;

    forwardPass(atividades, ordem_topologica);

    int duracao_projeto = 0;
    for (auto const& pair : atividades) {
        if (pair.second.sucessores.empty()) {
            duracao_projeto = max(duracao_projeto, pair.second.EF);
        }
    }
    cout << "--- Duração Mínima do Projeto: " << duracao_projeto << endl;

    backwardPass(atividades, ordem_topologica, duracao_projeto);

    return ordem_topologica; 
}

void exibirResultado(const map<string, Atividade>& atividades, const vector<string>& ordem_topologica) {
    cout << "\n## Resultados do PERT/CPM\n";
    cout << "--------------------------------------------------------------------------------" << endl;
    cout << "| Ativ | Dura | ES (Inicio Cedo) | EF (Fim Cedo) | LS (Inicio Tarde) | LF (Fim Tarde) | Folga | Critica |" << endl;
    cout << "--------------------------------------------------------------------------------" << endl;

    for (const string& id : ordem_topologica) {
        const Atividade& ativ = atividades.at(id);
        string critico = (ativ.folga == 0) ? "SIM" : "NAO";
        
        printf("| %4s | %4d | %16d | %13d | %17d | %14d | %5d | %7s |\n",
               ativ.id.c_str(), ativ.duracao, ativ.ES, ativ.EF, ativ.LS, ativ.LF, ativ.folga, critico.c_str());
    }
    cout << "--------------------------------------------------------------------------------" << endl;

    cout << "\n## Caminho Critico\n";
    cout << "Sequencia Critica: ";
    bool primeiro = true;

    for (const string& id : ordem_topologica) {
        if (atividades.at(id).folga == 0) {
            if (!primeiro) {
                cout << " -> ";
            }
            cout << id;
            primeiro = false;
        }
    }
    cout << endl;
}

void gerarArquivoDOT(const map<string, Atividade>& atividades, const vector<string>& ordem_topologica, const string& nome_arquivo) {
    ofstream arquivo(nome_arquivo);
    if (!arquivo.is_open()) {
        cerr << "Erro ao abrir o arquivo DOT para escrita." << endl;
        return;
    }

    arquivo << "digraph PERT_CPM {" << endl;
    arquivo << "  rankdir=LR;" << endl; 
    arquivo << "  overlap=false;" << endl; 
    arquivo << "  node [shape=record, fontname=\"Arial\"];" << endl; 

    string cor_critica_fundo = "mistyrose";
    string cor_normal_fundo = "lightblue";
    
    // Definição dos vértices (Atividades)
    for (const string& id : ordem_topologica) {
        const Atividade& ativ = atividades.at(id);
        string node_color = (ativ.folga == 0) ? cor_critica_fundo : cor_normal_fundo;
        string font_color = (ativ.folga == 0) ? "red" : "black";

        // Formato: { ID | DURA: X | { ES: X | EF: X } | { LS: X | LF: X } | FOLGA: X }
        arquivo << "  " << ativ.id << " [fillcolor=\"" << node_color << "\", style=filled, fontcolor=\"" << font_color << "\", label=\"{"
                << ativ.id << " | DURA: " << ativ.duracao << " | { ES: " << ativ.ES << " | EF: " << ativ.EF << " } | { LS: " << ativ.LS 
                << " | LF: " << ativ.LF << " } | FOLGA: " << ativ.folga 
                << "}\"];" << endl;
    }

    // Definição das arestas (Precedências)
    for (const string& id : ordem_topologica) {
        const Atividade& ativ = atividades.at(id);
        for (const string& sucessor_id : ativ.sucessores) {
            // Aresta crítica se ambos os vértices são críticos E há continuidade temporal
            bool is_critical_edge = (ativ.folga == 0 && atividades.at(sucessor_id).folga == 0 && ativ.EF == atividades.at(sucessor_id).ES);
            
            string edge_color = is_critical_edge ? "red" : "gray50";
            string edge_style = is_critical_edge ? "bold" : "solid";

            arquivo << "  " << ativ.id << " -> " << sucessor_id << " [color=\"" << edge_color << "\", style=" << edge_style << ", penwidth=" << (is_critical_edge ? 2.0 : 1.0) << "];" << endl;
        }
    }

    arquivo << "}" << endl;
    arquivo.close();
    cout << "\nArquivo DOT '" << nome_arquivo << "' gerado com sucesso!" << endl;
}

void gerarArquivoJSON(const map<string, Atividade>& atividades, const vector<string>& ordem_topologica, const string& nome_arquivo) {
    ofstream arquivo(nome_arquivo);
    if (!arquivo.is_open()) {
        cerr << "ERRO: Não foi possível abrir o arquivo JSON para escrita." << endl;
        return;
    }

    arquivo << "{" << endl;
    arquivo << "  \"projeto\": \"PERT_CPM_Grafo\"," << endl;
    
    // Calcula a duração total do projeto (maior LF das atividades finais)
    int duracao_total = 0;
    if (!ordem_topologica.empty()) {
        for (const string& id : ordem_topologica) {
            if (atividades.at(id).sucessores.empty()) {
                duracao_total = max(duracao_total, atividades.at(id).LF);
            }
        }
    }
    arquivo << "  \"duracao_total\": " << duracao_total << "," << endl; 
    
    arquivo << "  \"nodes\": [" << endl;

    // Geração dos vértices (Nodes)
    for (size_t i = 0; i < ordem_topologica.size(); ++i) {
        const string& id = ordem_topologica[i];
        const Atividade& ativ = atividades.at(id);

        arquivo << "    {" << endl;
        arquivo << "      \"data\": {" << endl;
        arquivo << "        \"id\": \"" << ativ.id << "\"," << endl;
        arquivo << "        \"label\": \"" << ativ.id << "\"," << endl;
        arquivo << "        \"duracao\": " << ativ.duracao << "," << endl;
        arquivo << "        \"es\": " << ativ.ES << "," << endl;
        arquivo << "        \"ef\": " << ativ.EF << "," << endl;
        arquivo << "        \"ls\": " << ativ.LS << "," << endl;
        arquivo << "        \"lf\": " << ativ.LF << "," << endl;
        arquivo << "        \"folga\": " << ativ.folga << "," << endl;
        arquivo << "        \"critica\": \"" << (ativ.folga == 0 ? "true" : "false") << "\"" << endl; // O valor true/false precisam ser concatenados com \" "true" \" para poderem ser escritos corretamente no JSON, e lidos pelo html
        arquivo << "      }" << endl;
        arquivo << "    }";
        if (i < ordem_topologica.size() - 1) {
            arquivo << ",";
        }
        arquivo << endl;
    }
    arquivo << "  ]," << endl;

    // Geração das Arestas (Edges)
    arquivo << "  \"edges\": [" << endl;
    bool primeira_aresta = true;

    for (const string& id : ordem_topologica) {
        const Atividade& ativ = atividades.at(id);
        for (const string& sucessor_id : ativ.sucessores) {
            
            if (!primeira_aresta) {
                arquivo << "," << endl;
            }
            
            bool is_critical_edge = (ativ.folga == 0 && atividades.at(sucessor_id).folga == 0 && ativ.EF == atividades.at(sucessor_id).ES);

            arquivo << "    {" << endl;
            arquivo << "      \"data\": {" << endl;
            arquivo << "        \"source\": \"" << ativ.id << "\"," << endl;
            arquivo << "        \"target\": \"" << sucessor_id << "\"," << endl;
            arquivo << "        \"critica\": \"" << (is_critical_edge ? "true" : "false") << "\"" << endl; // O valor true/false precisam ser concatenados com \" "true" \" para poderem ser escritos corretamente no JSON, e lidos pelo html
            arquivo << "      }" << endl;
            arquivo << "    }";

            primeira_aresta = false;
        }
    }
    
    if (!primeira_aresta) {
        arquivo << endl;
    }
    arquivo << "  ]" << endl;
    arquivo << "}" << endl;

    arquivo.close();
    cout << "\nArquivo JSON '" << nome_arquivo << "' gerado com sucesso. Use o index.html para visualizar." << endl;
}


int main() {
    // {"Nome", duração, precedentes("-" se não houver precedente, "A,B"Separar vários precedentes com vírgula)}:
    vector<tuple<string, int, string>> dados_projeto = {
        {"Z", 3, "-"},
        {"A", 2, "-"},
        {"B", 6, "K,L"},
        {"C", 10, "N,Z"},
        {"D", 6, "C"},
        {"E", 4, "C"},
        {"F", 5, "E"},
        {"G", 7, "D"},
        {"H", 9, "E,G"},
        {"I", 7, "C"},
        {"J", 8, "F,I"}, 
        {"K", 4, "J"},
        {"L", 5, "J"},
        {"M", 2, "H"},
        {"N", 4, "A"}
    };

    map<string, Atividade> atividades;
    string nome_arquivo_json = "grafo.json"; // Nome do arquivo 

    construirGrafo(atividades, dados_projeto);

    bool calculo_ok;
    vector<string> ordem_topologica = calcularPERT_CPM(atividades, calculo_ok);

    if (calculo_ok) {
        exibirResultado(atividades, ordem_topologica);
        
        gerarArquivoJSON(atividades, ordem_topologica, nome_arquivo_json);
    }

    return 0;
}