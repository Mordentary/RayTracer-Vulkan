#pragma once
#include "vector"
#include "string"
#include <optional>

namespace SE
{
	using DAGNodeID = uint32_t;
	class DirectedAcyclicGraph;
	class DAGNode;

	class DAGEdge
	{
		friend class DirectedAcyclicGraph;
	public:
		DAGEdge(DirectedAcyclicGraph& graph, DAGNode* from, DAGNode* to);
		virtual ~DAGEdge() = default;

		DAGNodeID GetFromNode() const { return m_from; }
		DAGNodeID GetToNode() const { return m_to; }

	private:
		const DAGNodeID m_from;
		const DAGNodeID m_to;
	};

	class DAGNode
	{
		friend class DirectedAcyclicGraph;
	public:
		DAGNode(DirectedAcyclicGraph& graph);
		virtual ~DAGNode() = default;

		DAGNodeID getID() const { return m_NodeID; }
		void markTarget() { m_RefCount = TARGET; }
		bool isTarget() const { return m_RefCount >= TARGET; }
		bool isCulled() const { return m_RefCount == 0; }
		uint32_t getRefCount() const { return isTarget() ? 1 : m_RefCount; }
	private:
		DAGNodeID m_NodeID;
		uint32_t m_RefCount = 0;
		static const uint32_t TARGET = 0x80000000u;
	};

	class DirectedAcyclicGraph
	{
	public:
		DAGNodeID generateNodeId() { return (DAGNodeID)m_Nodes.size(); }
		std::optional<DAGNode*> getNode(DAGNodeID id) const { return m_Nodes[id]; }
		std::optional<DAGEdge*> getEdge(DAGNodeID from, DAGNodeID to) const;

		void registerNode(DAGNode* node);
		void registerEdge(DAGEdge* edge);

		void clear();
		void cull();
		bool isEdgeValid(const DAGEdge* edge) const;
		void getIncomingEdges(const DAGNode* node, const std::vector<DAGEdge*>& edges) const;
		void getOutgoingEdges(const DAGNode* node, const std::vector<DAGEdge*>& edges) const;
	private:
		std::vector<DAGNode*> m_Nodes;
		std::vector<DAGEdge*> m_Edges;
	};
}