// Computes the disjoint Modularity of a given partition.
// Refer to eq(1) of https://arxiv.org/pdf/0910.0165.pdf for the definition
// Author: Satyaki Sikdar
// Email: satyaki.sikdar@gmail.com
// Date: March 19, 2017

#include<iostream>
#include<vector>
#include<unordered_map>
#include<unordered_set>
#include<fstream>
#include <cmath>
#include <chrono>
#include <sstream>
#include <cassert>

using namespace std;

class Graph
{
    unordered_map<int, unordered_set<int> > adj_list; // container for the adjacency list
    vector<pair<int, int> > edges; // container for the edge list
    unsigned int n, m; // n = |V|, m = |E|

    public:
    Graph()
    {}

    Graph(string filename)
    {
        // reads and initializes the Graph object
        ifstream fin(filename);
        if (!fin.is_open())
		{
			cerr << "File " << filename << " doesnt exist" << endl;
			exit(1);
		}
        int u, v;
        while (fin >> u >> v)
        {
            this -> adj_list[u].insert(v);
            this -> adj_list[v].insert(u);
            this -> edges.push_back(make_pair(u, v));
        }
        this -> n = adj_list.size();
        this -> m = edges.size();
    }

    unsigned int degree(int node)
    {
        // The number of neighbors of the node
        return this -> adj_list[node].size();
    }

    unordered_set<int> neighbors(int node)
    {
        // returns the set of neighbors of the node
        return this -> adj_list[node];
    }

    vector<pair<int, int> > get_edges()
    {
        // returns the edge list
        return this -> edges;
    }

    unsigned int size()
    {
        // returns |E|
        return this -> m;
    }

    unsigned int order()
    {
        // returns |V|
        return this -> n;
    }
};

Graph G;
bool verbose = false;
bool alternate_cover = false; // each line has all the cluster members ending with the flag

class Cluster
{
    unordered_set<int> members; // stores the member of the clusters
    unsigned int degree; // the degree of nodes in the cluster
    unsigned int num_edges; // the number of intra cluster edges

    public:
    Cluster()
    {
        this -> degree = 0;
        this  -> num_edges = 0;
    }

    void add_member(int node)
    {
        // adds a new member to the cluster, updates the cluster degree
        this -> members.insert(node);
        this -> degree += G.degree(node);
    }

    int get_degree()
    {
        // returns the sum of the degrees of the nodes in the cluster
        return this -> degree;
    }

    unordered_set<int> get_members()
    {
        // returns the members
        return this -> members;
    }

    void increase_edge_count()
    {
        // increases the edge count by 1
        this -> num_edges += 1;
    }

    int get_edge_count()
    {
        // returns the edge count
        return this -> num_edges;
    }
};

Graph read_edgelist(string filename)
{
    // reads an edgelist and makes a Graph
    Graph G(filename);
    return G;
}

pair<unordered_map<int, Cluster >, unordered_map<int, int> > read_cover(string cover)
{
    // reads a cover from a file and populates the data structures
    // returns the map of clusters and the map of community labels
    ifstream fin(cover);
    if (!fin.is_open())
    {
        cerr << "File " << cover << " doesnt exist" << endl;
        exit(1);
    }
    int node, comm_label;
    unordered_map<int, Cluster > clusters;
    unordered_map<int, int> community;

    if (! alternate_cover)
    {
        while(fin >> node >> comm_label)
        {
            clusters[comm_label].add_member(node);
            community[node] = comm_label;
        }
    }
    else
    {
        string line;
        int i = 0;
        while(getline(fin, line))
        {
            istringstream iss(line);
            int node;
            while (iss >> node)
            {
                if (node == -1)
                    break;
                clusters[i].add_member(node);
                community[node] = i;
            }
            i += 1;
        }
    }

    if (community.size() != G.order())
    {
        cout << "Incorrect cover! Terminating!" << endl;
        exit(1);
    }
    return make_pair(clusters, community);
}

double modularity(unordered_map<int, Cluster > clusters, unordered_map<int, int> community)
{
    // Returns the Modularity score of the partition
    // Note: The graph G is global
    vector<pair<int, int> > edges = G.get_edges();
    for (pair<int, int> edge: edges)
    {
        int u, v;
        tie(u, v) = edge;
        int u_comm = community.at(u);
        int v_comm = community.at(v);
        if (u_comm == v_comm) // intra community edge
        {
            clusters[u_comm].increase_edge_count();
        }
    }

    double mod = 0.0;
    int m = G.size();

    for (pair<int, Cluster> thing: clusters)
    {
        Cluster cluster = thing.second;
        mod += (double) cluster.get_edge_count() / m  - pow(((double)cluster.get_degree() / (2 * m)), 2);
    }
    return mod;
}


void parse_args(int argc, char const *argv[])
{
    // parses the command line args
    for (int i = 1; i < argc; i ++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
                case 'v':
                    verbose = true; // show timing info
                break;

                case 'a':
                    alternate_cover = true; // accept the alternate form of cover
                break;
            }
        }
    }
}

int main(int argc, char const *argv[])
{
    parse_args(argc, argv);

    auto start = chrono::high_resolution_clock::now();
    G = read_edgelist(argv[1]);
    auto graph_end = chrono::high_resolution_clock::now();

    cout << "n = " << G.order() << ", m = " << G.size() << endl;

    unordered_map<int, Cluster > clusters;
    unordered_map<int, int> community;

    auto cover_read_start = chrono::high_resolution_clock::now();
    tie(clusters, community) = read_cover(argv[2]);
    auto cover_read_end = chrono::high_resolution_clock::now();

    cout << "Read " << clusters.size() << " clusters " << endl;

    auto mod_start = chrono::high_resolution_clock::now();
    double mod = modularity(clusters, community);
    auto mod_end = chrono::high_resolution_clock::now();

    cout << "Modularity of the partition: " << mod << endl;

    if (verbose)
    {
        cout << endl;
        std::chrono::duration<double> time_elapsed = graph_end - start;
        cout << "Graph is read in " << time_elapsed.count() << " seconds" << endl;
        time_elapsed = cover_read_end - cover_read_start;
        cout << "Cover is read in " << time_elapsed.count() << " seconds" << endl;
        time_elapsed = mod_end - mod_start;
        cout << "Modularity calculated in " << time_elapsed.count() << " seconds" << endl;
        time_elapsed = mod_end - start;
        cout << "Total time taken: " << time_elapsed.count() << " seconds" << endl;
    }
    return 0;
}
