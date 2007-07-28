/*
	This file is part of Warzone 2100.
	Copyright (C) 1999-2004  Eidos Interactive
	Copyright (C) 2005-2007  Warzone Resurrection Project

	Warzone 2100 is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	Warzone 2100 is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Warzone 2100; if not, write to the Free Software
	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA

	$Revision$
	$Id$
	$HeadURL$
*/

#ifndef __INCLUDED_LISTTEMP__
#define __INCLUDED_LISTTEMP__

#include <cassert>

template<class T>
class ListNode
{
	public:
		ListNode() :
			PrevNode(NULL),
			NextNode(NULL),
			Data(new T)
		{
		}

		~ListNode()
		{
		//	DebugPrint("DeleteNode\n");
			delete Data;
		}

		T* GetData()
		{
			return Data;
		}

		void AppendNode(ListNode<T>* RootNode)
		{
			ListNode<T>* TmpNode = RootNode->GetLastNode(); // Find the end of the list.

			TmpNode->NextNode = this; // Append the new node to the list.

			PrevNode = TmpNode;
			NextNode = NULL;
		}

		void InsertAfterNode(ListNode<T>* Node)
		{
			PrevNode = Node;
			NextNode = Node->NextNode;

			Node->NextNode = this;
			if(NextNode != NULL)
				NextNode->PrevNode = this;
		}

		void InsertBeforeNode(ListNode<T>* Node)
		{
			PrevNode = Node->PrevNode;
			Node->PrevNode = this;

			NextNode = Node;
			if(PrevNode != NULL)
				PrevNode->NextNode = this;
		}

		ListNode<T>* GetNextNode()
		{
			return NextNode;
		}

		ListNode<T>* GetPrevNode()
		{
			return PrevNode;
		}

		ListNode<T>* GetFirstNode()
		{
			ListNode* TmpNode = this;							// Get address of this node.
			while(TmpNode->PrevNode != NULL) // Find the start of the list.
			{
				TmpNode = TmpNode->PrevNode;
			}

			return TmpNode;
		}

		ListNode<T>* GetLastNode()
		{
			ListNode<T>* TmpNode = this;							// Get address of this node.
			while(TmpNode->NextNode != NULL) // Find the end of the list.
			{
				TmpNode = TmpNode->NextNode;
			}

			return TmpNode;
		}

		ListNode<T>* GetNthNode(size_t NodeNum)
		{
			ListNode<T>* TmpNode = GetFirstNode(); // Find the start of the list.
			while (TmpNode != NULL)
			{
				if(NodeNum == 0)
					return TmpNode;

				TmpNode = TmpNode->NextNode;
				--NodeNum;
			}

			return NULL;
		}

		ListNode<T>* RemoveNode(ListNode<T>* RootNode)
		{
		//	DebugPrint("* Removing node @ %p\n",this);

			if(this == RootNode)
			{
				RootNode = RootNode->GetNextNode();
		//		DebugPrint("* Root changed %p\n",RootNode);
			}

			if(PrevNode != NULL)
				PrevNode->NextNode = NextNode;

			if(NextNode != NULL)
				NextNode->PrevNode = PrevNode;

			return RootNode;
		}

		void DeleteList()
		{
			ListNode<T>* TmpNode = GetFirstNode(); // Find the start of the list.

			ListNode<T>* TmpNode2;
			while(TmpNode != NULL) // Delete all items in the list.
			{
				TmpNode2 = TmpNode;
				TmpNode = TmpNode->NextNode;
				delete TmpNode2;
			}
		}

		size_t CountNodes()
		{
			ListNode<T>* TmpNode = GetFirstNode(); // Find the start of the list.
			size_t count = 0;

			// Walk to the end of the list and count all encountered nodes
			while(TmpNode != NULL)
			{
				TmpNode = TmpNode->NextNode;
				++count;
			}

			return count;
		}

		class iterator
		{
			public:
				iterator(ListNode<T>* ptr = NULL) :
					_nodePtr(ptr)
				{}

				iterator(const iterator& rhs) :
					_nodePtr(rhs._nodePtr)
				{}

				iterator& operator=(const iterator& rhs)
				{
					_nodePtr = rhs._nodePtr;

					return *this;
				}

				bool operator==(const iterator& rhs) const
				{
					return _nodePtr == rhs._nodePtr;
				}

				bool operator!=(const iterator& rhs) const
				{
					return !(*this == rhs);
				}

				T& operator*() const
				{
					assert(_nodePtr != NULL && _nodePtr->GetData() != NULL);
					return *_nodePtr->GetData();
				}

				T* operator->() const
				{
					assert(_nodePtr != NULL && _nodePtr->GetData() != NULL);
					return _nodePtr->GetData();
				}

				iterator& operator++()
				{
					_nodePtr = _nodePtr->GetNextNode();

					return *this;
				}

				iterator operator++(int)
				{
					iterator tmp(*this);
					++(*this);
					return tmp;
				}

				void goToBegin()
				{
					_nodePtr = _nodePtr->GetFirstNode();
				}

			private:
				ListNode<T>* _nodePtr;
		};

	protected:
		ListNode<T>* PrevNode;
		ListNode<T>* NextNode;
		T* Data;
};

#endif
