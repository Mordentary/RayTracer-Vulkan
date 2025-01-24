#include "directed_acyclic_graph.hpp"

namespace SE
{
	DAGEdge::DAGEdge(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to)
		: m_From(from->getId())
		, m_To(to->getId())
	{
		SE_ASSERT(graph.getNode(m_From) == from);
		SE_ASSERT(graph.getNode(m_To) == to);
		graph.registerEdge(this);
	}

	DAGNode::DAGNode(DirectedAcyclicGraph& graph)
	{
		m_NodeId = graph.generateNodeId();
		graph.registerNode(this);
	}

	std::optional<DAGEdge*> DirectedAcyclicGraph::getEdge(DAGNodeID from, DAGNodeID to) const
	{
		for (size_t i = 0; i < m_Edges.size(); ++i)
		{
			if (m_Edges[i]->m_From == from && m_Edges[i]->m_To == to)
			{
				return m_Edges[i];
			}
		}
		return std::nullopt;
	}

	void DirectedAcyclicGraph::registerNode(DAGNode* node)
	{
		SE_ASSERT(node->getId() == m_Nodes.size());
		m_Nodes.push_back(node);
	}

	void DirectedAcyclicGraph::registerEdge(DAGEdge* edge)
	{
		m_Edges.push_back(edge);
	}

	void DirectedAcyclicGraph::clear()
	{
		m_Edges.clear();
		m_Nodes.clear();
	}

	void DirectedAcyclicGraph::cull()
	{
		// update reference counts
		for (size_t i = 0; i < m_Edges.size(); ++i)
		{
			DAGEdge* edge = m_Edges[i];
			DAGNode* node = m_Nodes[edge->m_From];
			node->m_RefCount++;
		}

		// cull nodes with a 0 reference count
		std::vector<DAGNode*> stack;
		for (size_t i = 0; i < m_Nodes.size(); ++i)
		{
			if (m_Nodes[i]->getRefCount() == 0)
			{
				stack.push_back(m_Nodes[i]);
			}
		}

		// cull the entire chain
		while (!stack.empty())
		{
			DAGNode* node = stack.back();
			stack.pop_back();

			std::vector<DAGEdge*> incoming;
			getIncomingEdges(node, incoming);

			for (size_t i = 0; i < incoming.size(); ++i)
			{
				auto optNode = getNode(incoming[i]->m_From);
				SE_ASSERT(optNode.has_value(), "Graph node is null!");

				DAGNode* linked_node = optNode.value();

				if (--linked_node->m_RefCount == 0)
				{
					stack.push_back(linked_node);
				}
			}
		}
	}

	bool DirectedAcyclicGraph::isEdgeValid(const DAGEdge* edge) const
	{
		return !getNode(edge->m_From).value()->isCulled() &&
			!getNode(edge->m_To).value()->isCulled();
	}

	void DirectedAcyclicGraph::getIncomingEdges(const DAGNode* node, std::vector<DAGEdge*>& edges) const
	{
		edges.clear();
		for (size_t i = 0; i < m_Edges.size(); i++)
		{
			auto& edge = m_Edges[i];
			if (edge->m_To == node->getId())
			{
				edges.push_back(edge);
			}
		}
	}

	void DirectedAcyclicGraph::getOutgoingEdges(const DAGNode* node, std::vector<DAGEdge*>& edges) const
	{
		edges.clear();
		for (size_t i = 0; i < m_Edges.size(); ++i)
		{
			auto& edge = m_Edges[i];
			if (edge->m_From == node->getId())
			{
				edges.push_back(edge);
			}
		}
	}
}