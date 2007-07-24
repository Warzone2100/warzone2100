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

template<class T>
class	ListNode {
protected:
	ListNode<T> *PrevNode;
	ListNode<T> *NextNode;
	T *Data;
public:
	ListNode();
	~ListNode();
	T* GetData(void) { return Data; }
	void AppendNode(ListNode<T>* RootNode);
	void InsertAfterNode(ListNode<T>* Node);
	void InsertBeforeNode(ListNode<T>* Node);
	ListNode<T>* GetNextNode(void);
	ListNode<T>* GetPrevNode(void);
	ListNode<T>* GetFirstNode(void);
	ListNode<T>* GetLastNode(void);
	ListNode<T>* GetNthNode(int NodeNum);
	ListNode<T>* RemoveNode(ListNode<T>* RootNode);
	void DeleteList(void);
	int CountNodes(void);
};

template<class T>
ListNode<T>::ListNode()
{
	PrevNode=NULL;
	NextNode=NULL;
	Data=new T;
}

template<class T>
ListNode<T>::~ListNode()
{
//	DebugPrint("DeleteNode\n");
	delete Data;
}


template<class T>
void ListNode<T>::DeleteList(void)
{
	ListNode<T>* TmpNode;

	TmpNode=this;							// Get address of this node.
	while(TmpNode->PrevNode!=NULL) {		// Find the start of the list.
		TmpNode=TmpNode->PrevNode;
	}

	ListNode<T>* TmpNode2;
	while(TmpNode!=NULL) {		// Delete all items in the list.
		TmpNode2=TmpNode;
		TmpNode=TmpNode->NextNode;
		delete TmpNode2;
	}
}

template<class T>
ListNode<T>* ListNode<T>::RemoveNode(ListNode<T>* RootNode)
{
//	DebugPrint("* Removing node @ %p\n",this);

	if(this==RootNode) {
		RootNode=RootNode->GetNextNode();
//		DebugPrint("* Root changed %p\n",RootNode);
	}

	if(PrevNode!=NULL) {
		PrevNode->NextNode=NextNode;
	}
	if(NextNode!=NULL) {
		NextNode->PrevNode=PrevNode;
	}

	return RootNode;
}

template<class T>
void ListNode<T>::AppendNode(ListNode<T>* RootNode)
{
	ListNode<T>* TmpNode;

	TmpNode=RootNode;
	while(TmpNode->NextNode!=NULL) {	// Find the end of the list.
		TmpNode=TmpNode->NextNode;
	}

	TmpNode->NextNode=this;	// Append the new node to the list.

	PrevNode=TmpNode;
	NextNode=NULL;
}


template<class T>
void ListNode<T>::InsertAfterNode(ListNode<T>* Node)
{
	PrevNode=Node;
	NextNode=Node->NextNode;

	Node->NextNode=this;
	if(NextNode!=NULL) {
		NextNode->PrevNode=this;
	}
}


template<class T>
void ListNode<T>::InsertBeforeNode(ListNode<T>* Node)
{
	PrevNode=Node->PrevNode;
	Node->PrevNode=this;

	NextNode=Node;
	if(PrevNode!=NULL) {
		PrevNode->NextNode=this;
	}
}


template<class T>
ListNode<T>* ListNode<T>::GetNextNode(void)
{
	return(NextNode);
}


template<class T>
ListNode<T>* ListNode<T>::GetPrevNode(void)
{
	return(PrevNode);
}


template<class T>
ListNode<T>* ListNode<T>::GetFirstNode(void)
{
	ListNode	*TmpNode;

	TmpNode=this;							// Get address of this node.
	while(TmpNode->PrevNode!=NULL) {		// Find the start of the list.
		TmpNode=TmpNode->PrevNode;
	}

	return(TmpNode);
}


template<class T>
ListNode<T>* ListNode<T>::GetLastNode(void)
{
	ListNode<T>* TmpNode;

	TmpNode=this;							// Get address of this node.
	while(TmpNode->NextNode!=NULL) {		// Find the end of the list.
		TmpNode=TmpNode->NextNode;
	}

	return(TmpNode);
}


template<class T>
ListNode<T>* ListNode<T>::GetNthNode(int NodeNum)
{
	ListNode<T>* TmpNode;

	TmpNode=this;							// Get address of this node.
	while(TmpNode->PrevNode!=NULL) {		// Find the start of the list.
		TmpNode=TmpNode->PrevNode;
	}


	while(TmpNode!=NULL) {
		if(NodeNum==0) {
			return(TmpNode);
		}
		TmpNode=TmpNode->NextNode;
		NodeNum--;
	}

	return(NULL);
}


template<class T>
int ListNode<T>::CountNodes(void)
{
	int	count;
	ListNode<T>* TmpNode;

	TmpNode=this;							// Get address of this node.
	while(TmpNode->PrevNode!=NULL) {		// Find the start of the list.
		TmpNode=TmpNode->PrevNode;
	}

	count=0;
	while(TmpNode!=NULL) {		// Find the end of the list.
		TmpNode=TmpNode->NextNode;
		count++;
	}

	return(count);
}

#endif
