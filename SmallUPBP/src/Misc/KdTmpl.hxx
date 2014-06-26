/*
 * Copyright (C) 2014, Petr Vevoda, Martin Sik (http://cgg.mff.cuni.cz/~sik/), 
 * Tomas Davidovic (http://www.davidovic.cz), Iliyan Georgiev (http://www.iliyan.com/), 
 * Jaroslav Krivanek (http://cgg.mff.cuni.cz/~jaroslav/)
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * (The above is MIT License: http://en.wikipedia.origin/wiki/MIT_License)
 */

#ifndef __KDTMPL_HXX__
#define __KDTMPL_HXX__

// standard headers
#include <vector>

/// Kd-tree data structure by Henrik Wann Jensen.
/// Modified for general usage. 
/// BEWARE!!! This tree does not store the data items, but only pointers to them!
template<class T, class TVec3>
class KdTreeTmplPtr {

public:

  // true predicate
  class CTrue { public:   bool operator() (const T& s) const {return true; } } truePred;

  class CTreeNode {
  protected:
    T *pt;
	unsigned int index;
    TVec3 P;
    int plane;
  public:
    CTreeNode() {}
    CTreeNode(T *ppt, unsigned int i) : pt(ppt), index(i), P(*ppt) {}
    const TVec3& GetP() const {return P;}
    float GetP(int i)       const {return GetP()[i];}
    int   GetPlane()        const {return plane;}
    T* GetPt()              const {return  pt;}
	unsigned int GetIndex()	const {return index;}
    float SplitVal()        const {return GetP()[plane];}
    void  SetPlane(int p)   {plane = p;}
  };

  /// Pointer to data point and it's distance from the query point.
  class CPointAndDist {
  public:
    T      *pt;
    float  d2;
    CPointAndDist() {}
    CPointAndDist(T *ppt,const float &pd2) : 
    d2(pd2), pt(ppt) {}
  };

  /// Nearest point query structure.
  class CNearestQuery : public CPointAndDist {
  public:
    /// search location
    TVec3 P;
    CNearestQuery(const TVec3& p,const float pd2=1e10,T *ppt=NULL):
    CPointAndDist(ppt,pd2),P(p) {}
	void SetPt(T *ppt, const float pd2) { CPointAndDist::pt=ppt; CPointAndDist::d2=pd2; }
  };

  /// Range query structure (all points in a given radius).
  template<class Tlist>
  class CRangeQuery {
  protected: // data
    TVec3 _P;
    float _r;
    float _r2;
  public: //data
    Tlist  list;
  public: // methods
    // default constructor
    CRangeQuery() {}
    /// init search
    void Init(const TVec3 &P,float r)
    {
      list.clear();
      _P = P;
      _r = r;
      _r2 = r*r;
    }
    /// return search position
    const TVec3& GetP() const {return _P;}
    /// return search position component
    float GetP(int i) const {return _P[i];}
    /// return search radius
    float GetR() const {return _r;}
    /// return radius squared
    float GetR2() const {return _r2;}
  };

  /// K-NN query structure (N nearest points not farther than the given initial radius).
  class CKNNQuery
  {
  public:
    TVec3 pos;
    int       max;
    int       found;
    int       got_heap;
    float     *dist2;
    const     T **index;
	unsigned int *indeces;

    CKNNQuery(int maxGatherCount)
    {
      dist2 = new float [maxGatherCount + 1];
      index = new T const*[maxGatherCount + 1];
	  indeces = new unsigned int [maxGatherCount + 1];
    }
    ~CKNNQuery()
    {
      delete [] dist2; delete [] index; delete[] indeces;
    }

    inline void Init(const TVec3 &p, int maxphotons, float initrad) 
    {
      pos      = p; 
      max      = maxphotons; 
      dist2[0] = initrad*initrad;
      found    = 0; 
      got_heap = 0;
    }
  };


  /// Constructor
  KdTreeTmplPtr() { Clear(); }

  /// Destructor
  ~KdTreeTmplPtr();

  /// Puts a new data item into the flat array that will form the final kd-tree.
  inline void AddItem(T* pt, unsigned int index);

  /// Reserve space for a given number of points
  inline void Reserve(size_t count) {_points.reserve(count+1);}

  /// Empty the tree and reninit it.
  inline void Clear();

  /// Empty the tree and reninit it.
  inline void RemoveItems() 
  {   
    _numPoints = _halfNumPoints = 0;
    _bbox_min.SetValue(1e8f);
    _bbox_max.SetValue(-1e8f);
  }

  /// init tree for a given array (clear the tree, copy items and build up)
  void Init( T* array, size_t length );

  /// Creates a left balanced kd-tree from the flat data array.
  /**
     This function should be called before the kd-tree is first queried.
     It balances kd-tree before searching.
  */
  void BuildUp();
							   
  /// Finds items within a given radius from the center.
  template<class Tlist>
    void RangeQuery(CRangeQuery<Tlist> &np) const;

  /// Finds items within a given radius from the center for which a predicate is true.
  template<class Tlist,class Predicate>
    void RangeQueryIf(CRangeQuery<Tlist> &np,const Predicate &pred) const;

  /// Finds k nearest neighbors within a given radius from the center.
  template<class Predicate>
  void KNNQuery(CKNNQuery &np,const Predicate &pred) const;

  /// Returns the single nearest point in the tree
  template<class Predicate>
    void FindNearestIf(CNearestQuery &np, int index, 
    const Predicate &pred) const;

  /// Returns the single nearest point in the tree - use linear search
  template<class Predicate>
    void FindNearestIfLinear(CNearestQuery &np, int index, 
    const Predicate &pred) const;

  /// Traverse all nodes in depth-first order.
  template<class Tlist>
    void TraverseDF(Tlist &list) const;

	template<class Tlist>
	void TraverseDFInfix(Tlist &list, int index=1) const;

  /// Traverse all nodes in breadth-first order.
  template<class Tlist>
    void TraverseBF(Tlist &list) const;

  int GetNumPoints() const { return _numPoints; }

  const T* GetPoint(int i) const { return _points[i+1].pt; }
        T* GetPoint(int i)       { return _points[i+1].pt; }

protected: // methods

  /**
     See "Realistic image synthesis using Photon Mapping" chapter 6
     for an explanation of this function.
  */
  void _balanceSegment(
    // CTreeNode *pbal,
    // CTreeNode *porg,
    const int index,
    const int start,
    const int end);

  /**
    Median_split splits the point array into two separate
    pieces around the median with all points below the
    the median in the lower half and all points above
    than the median in the upper half. The comparison
    criteria is the axis (indicated by the axis parameter)
    (inspired by routine in "Algorithms in C++" by Sedgewick)
  */
  template<int axis>
  void _medianSplit(CTreeNode *p,const int start,const int end,const int med);

  /// check the distance of the photon and adds it to the resulting array
  template<class Predicate>
  static inline void _checkAddNearest(CKNNQuery *np, const CTreeNode &p, float &maxDistSqr,
    const Predicate &pred);

protected: // data

  /// point array
  // std::vector<T*> _points;
  std::vector<CTreeNode> _points;

  /// number of stored points
  int _numPoints, _halfNumPoints;

  /// point bbox
  TVec3 _bbox_min;
  TVec3 _bbox_max;

  CTreeNode *pbal;
  CTreeNode *porg;

private:  // Just for some stuff no longer within the headers
  float SqrDistance(TVec3 p1, TVec3 p2) const {
    const float x = p1.x - p2.x;
	const float y = p1.y - p2.y;
	const float z = p1.z - p2.z;
    return x*x + y*y + z*z;
  }

};

// --------------------------------------------------------------------
//  KdTreeTmplPtr::Clear()
// --------------------------------------------------------------------
template<class T, class TVec3>
inline void
KdTreeTmplPtr<T,TVec3>::Clear()
{
  _numPoints = _halfNumPoints = 0;
  _points.clear();
  // This seems illogical but it is necessary due to the implementation
  // details of Jensen Photon Map. This photon is not used anywhere.
  _points.push_back(CTreeNode());
  // Initiate the size of the box containing all the photons.
  _bbox_min.set(1e18f,1e18f,1e18f);
  _bbox_max.set(-1e18f,-1e18f,-1e18f);
}

// --------------------------------------------------------------------
//  KdTreeTmplPtr::~KdTreeTmplPtr()
// --------------------------------------------------------------------
template<class T, class TVec3>
KdTreeTmplPtr<T,TVec3>::~KdTreeTmplPtr()
{
}

// --------------------------------------------------------------------
//   KdTreeTmplPtr::AddItem()
// --------------------------------------------------------------------
template<class T, class TVec3>
inline void
KdTreeTmplPtr<T,TVec3>::AddItem(T* pt, unsigned int index)
{
  _numPoints++;
  _points.push_back(CTreeNode(pt, index));
  // Update minima and maxima
  for (int i=0; i<3; i++) {
    if ((*pt)[i] < _bbox_min[i])
      _bbox_min[i] = (*pt)[i];
    if ((*pt)[i] > _bbox_max[i])
      _bbox_max[i] = (*pt)[i];
  } // for
}

// --------------------------------------------------------------------
//   KdTreeTmplPtr::BuildUp()
// --------------------------------------------------------------------
template<class T, class TVec3>
void
KdTreeTmplPtr<T,TVec3>::BuildUp(void)
{
  if (_numPoints>1) 
  {
    // allocate two temporary arrays for the balancing procedure
    pbal = new CTreeNode[_numPoints+1];
    porg = &_points[0];
    // balance tree
    _balanceSegment( 1, 1, _numPoints);
    memmove(&_points[0], pbal, (_numPoints+1)*sizeof(CTreeNode));
    delete [] pbal;
    pbal = porg = 0;
  }
  _halfNumPoints = _numPoints / 2 - 1;
}

// --------------------------------------------------------------------
template<class T, class TVec3>
void 
KdTreeTmplPtr<T,TVec3>::Init( T* array, size_t length )
{
	Clear();

	Reserve(length);

	for(size_t i=0; i<length; i++) 
		AddItem(array+i);

	BuildUp();
}

// --------------------------------------------------------------------
//  KdTreeTmplPtr::_medianSplit()
// --------------------------------------------------------------------
template<class T, class TVec3>
template<int axis>
void
KdTreeTmplPtr<T,TVec3>::_medianSplit(CTreeNode *p,
                                const int start,
                                const int end,
                                const int median)
{
#define _msswap(ph,a,b) { CTreeNode ph2=ph[a]; ph[a]=ph[b]; ph[b]=ph2; }
  int left = start;
  int right = end;
  while ( right > left ) {
    const float v = p[right].GetP(axis);
    int i = left - 1;
    int j = right;
    for (;;) {
      while ( p[++i].GetP(axis) < v )
        ;
      while ( (p[--j].GetP(axis) > v) && (j > left) )
        ;
      if ( i >= j )
        break;
      _msswap(p, i, j);
    } // for

    _msswap(p, i, right);
    if ( i >= median )
      right = i - 1;
    if ( i <= median )
      left = i + 1;
  } // while
#undef _msswap
}

// --------------------------------------------------------------------
//  KdTreeTmplPtr::_balanceSegment()
// --------------------------------------------------------------------
template<class T, class TVec3>
void
KdTreeTmplPtr<T,TVec3>::_balanceSegment(
  // CTreeNode *pbal,
  // CTreeNode *porg,
  const int index,
  const int start,
  const int end )
{
  // static int _balanceSegmentCallCounter=0;
  // int callNum=_balanceSegmentCallCounter++;


  // compute new median
  int median=1;
  // find the largest power of two smaller than (end-start+1)
  while ( (4 * median) <= (end - start + 1))
    median += median;

  if ((3 * median) <= (end - start + 1)) {
    median += median;
    median += start-1;
  }
  else	
    median = end - median + 1;

  // find axis to split along
  int axis = 2;
  if ((_bbox_max[0]-_bbox_min[0])>(_bbox_max[1]-_bbox_min[1]) &&
      (_bbox_max[0]-_bbox_min[0])>(_bbox_max[2]-_bbox_min[2]))
    axis = 0;
  else
    if ((_bbox_max[1]-_bbox_min[1])>(_bbox_max[2]-_bbox_min[2]))
      axis=1;

  // partition photon block around the median
  switch(axis) {
    case 0: _medianSplit<0>( porg, start, end, median ); break;
    case 1: _medianSplit<1>( porg, start, end, median ); break;
    case 2: _medianSplit<2>( porg, start, end, median ); break;
  }

  pbal[ index ] = porg[ median ];
  pbal[ index ].SetPlane( axis );
	
  // recursively balance the left and right block
	
  if ( median > start ) {
    // balance left segment
    if ( start < median-1 ) {
      const float tmp=_bbox_max[axis];
      _bbox_max[axis] = pbal[index].GetP(axis);
      // DEBUG << "going left from " << callNum << std::endl;
      _balanceSegment( /*pbal, porg,*/ 2*index, start, median-1 );
      // DEBUG << "return from left in " << callNum << std::endl;
      _bbox_max[axis] = tmp;
    }
    else {
      pbal[ 2*index ] = porg[start];
    }
  }
	
  if ( median < end ) {
    // balance right segment
    if ( median + 1 < end ) {
      const float tmp = _bbox_min[axis];		
      _bbox_min[axis] = pbal[index].GetP(axis);
      // DEBUG << "going right from " << callNum << std::endl;
      _balanceSegment( /*pbal, porg,*/ 2 * index + 1, median + 1, end );
      // DEBUG << "return from right in " << callNum << std::endl;
      _bbox_min[axis] = tmp;
    }
    else {
      pbal[ 2 * index + 1 ] = porg[end];
    }
  }

  // DEBUG << "finishing " << callNum << std::endl;
  return;
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Tlist>
void 
KdTreeTmplPtr<T,TVec3>::RangeQuery(CRangeQuery<Tlist> &np) const
{
  int stack[100];
  stack[0] = 1;
  int stackTop = 1;

  while(stackTop>0) {
    int index = stack[--stackTop];
    const CTreeNode &node = _points[index];

    float dist2 = SqrDistance(node.GetP(),np.GetP());
    if ( dist2 <= np.GetR2() )
      np.list.push_back(CPointAndDist(node.GetPt(),dist2));

    int childindex;
    if((childindex=2*index+1) <= _numPoints &&
      np.GetP(node.GetPlane()) + np.GetR() >= node.SplitVal())
      stack[stackTop++] = childindex;  // go right
    if((childindex=2*index) <= _numPoints && 
      np.GetP(node.GetPlane()) - np.GetR() <= node.SplitVal())
      stack[stackTop++] = childindex;    // go left
  }
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Tlist,class Predicate>
void 
KdTreeTmplPtr<T,TVec3>::RangeQueryIf(CRangeQuery<Tlist> &np,
                                const Predicate &pred) const
{
  int stack[100];
  stack[0] = 1;
  int stackTop = 1;

  while(stackTop>0) {
    int index = stack[--stackTop];
    const CTreeNode &node = _points[index];

    float dist2 = SqrDistance(node.GetP(),np.GetP());
    if ( dist2 <= np.GetR2() && pred(*node.GetPt()) )
      np.list.push_back(CPointAndDist(node.GetPt(),dist2));

    int childindex;
    if((childindex=2*index+1) <= _numPoints &&
      np.GetP(node.GetPlane()) + np.GetR() >= node.SplitVal())
      stack[stackTop++] = childindex;  // go right
    if((childindex=2*index) <= _numPoints && 
      np.GetP(node.GetPlane()) - np.GetR() <= node.SplitVal())
      stack[stackTop++] = childindex;    // go left
  }
}

// --------------------------------------------------------------------
//  KdTreeTmplPtr<T,TVec3>::FindNearestIf()
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Predicate>
void
KdTreeTmplPtr<T,TVec3>::FindNearestIf(CNearestQuery &np, int index, const Predicate &pred) const
{
#if 1
  const CTreeNode &node = _points[index];
  int leftChild=2*index;

  if(leftChild<=_numPoints) 
  {
    const float d1 = node.SplitVal() - np.P[node.GetPlane()];
    if(leftChild+1<=_numPoints) 
    {
      if( d1>0 ) 
      {
        FindNearestIf(np,leftChild,pred); // go left fist
        if( np.d2 >= d1*d1 )
          FindNearestIf(np,leftChild+1,pred); // go right second
      }
      else 
      {
        FindNearestIf(np,leftChild+1,pred); // go right first
        if( np.d2 >= d1*d1 )
          FindNearestIf(np,leftChild,pred); // go left second
      }
    }
    else 
    {
      if( np.d2 >= d1*d1 )
        FindNearestIf(np,leftChild,pred); // go only left
    }
  }

  float dist2 = SqrDistance(node.GetP(),np.P);
  if ( dist2 < np.d2 && pred(*node.GetPt())) np.SetPt(node.GetPt(),dist2);

#else

  for(int i=1; i<=_numPoints; i++) {
    const CTreeNode &node = _points[i];
      float dist2 = SqrDistance(node.GetP(),np.P);
      if ( dist2 < np.d2 && pred(*node.GetPt())) np.SetPt(node.GetPt(),dist2);
  }

#endif
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Predicate>
void
KdTreeTmplPtr<T,TVec3>::FindNearestIfLinear(CNearestQuery &np, int index, const Predicate &pred) const
{
  for(int i=index; i<=_numPoints; i++) {
    const CTreeNode &node = _points[i];
      float dist2 = SqrDistance(node.GetP(),np.P);
      if ( dist2 < np.d2 && pred(*node.GetPt())) np.SetPt(node.GetPt(),dist2);
  }
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Predicate>
inline void
KdTreeTmplPtr<T,TVec3>::_checkAddNearest(CKNNQuery *np, const CTreeNode &p, float &maxDistSqr,
                                    const Predicate &pred)
{
  // compute squared distance between current photon and (query point)
  float dist1 = p.GetP(0) - np->pos[0];
  float dist2 = dist1 * dist1;
  dist1 = p.GetP(1) - np->pos[1];
  dist2 += dist1 * dist1;
  dist1 = p.GetP(2) - np->pos[2];
  dist2 += dist1 * dist1;
  
  if ( dist2 < np->dist2[0] && pred(*p.GetPt()) ) 
  {
    // we found a photon  [:)] Insert it in the candidate list

    if ( np->found < np->max ) 
    {
      // heap is not full; use array sequentially
      np->found++;
      np->dist2[np->found] = dist2;
      np->index[np->found] = p.GetPt();
	  np->indeces[np->found] = p.GetIndex();
      if (maxDistSqr < dist2)
        maxDistSqr = dist2;
    }
    else 
    {
      int j, parent;

      if (np->got_heap == 0) { // Do we need to build the heap?
        // Build heap
        float dst2;
        const T *phot;
		unsigned int index;
        int half_found = np->found >> 1;
        for ( int k = half_found; k >= 1; k--) {
          parent = k;
          phot = np->index[k];
		  index = np->indeces[k];
          dst2 = np->dist2[k];
          while ( parent <= half_found ) {
            j = parent + parent;
            if ( (j < np->found) &&
              (np->dist2[j] < np->dist2[j+1]) )
              j++;
            if (dst2>=np->dist2[j])
              break;
            np->dist2[parent] = np->dist2[j];
            np->index[parent] = np->index[j];
			np->indeces[parent] = np->indeces[j];
            parent=j;
          }
          np->dist2[parent] = dst2;
          np->index[parent] = phot;
		  np->indeces[parent] = index;
        }
        np->got_heap = 1;
        // the heap was built for the first time
      }

      // insert new photon into max heap
      // delete largest element, insert new and reorder the heap
      parent = 1;
      j = 2;
      while ( j <= np->found ) 
      {
        if ( (j < np->found) && (np->dist2[j] < np->dist2[j+1]) )
          j++;
        if ( dist2 > np->dist2[j] )
          break;
        np->dist2[parent] = np->dist2[j];
        np->index[parent] = np->index[j];
		np->indeces[parent] = np->indeces[j];
        parent = j;
        j += j;
      } // while
      
      np->index[parent] = p.GetPt();
	  np->indeces[parent] = p.GetIndex();
      np->dist2[parent] = dist2;

      // update the maximum square distance to the farthest point in the result
      np->dist2[0] = maxDistSqr = np->dist2[1];
    } // if np->max >= np->found
  } // if (dist2 > np->dist2[0])

  return;
}


// --------------------------------------------------------------------
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Predicate>
void 
KdTreeTmplPtr<T,TVec3>::KNNQuery(CKNNQuery &np,const Predicate& pred) const
{
 // maximum square distance
  float maxDistSqr = 0.0; 
  // index in the heap of photons
  int index = 1;
  // current stack pointer
  int level = 0;
  // stack entry
  struct SSTackEntry 
  {
    int   index; // the index to the leaf to be processed
    float dist2; // the square distance computed
  };
  // maximum depth of the tree
  static const int MAX_DEPTH = 64;
  SSTackEntry stack[MAX_DEPTH];

  // traverse the whole tree
  while (true) {
    // Traverse down the tree to the first leaf
    while (index < _halfNumPoints) 
    {
      const CTreeNode &p = _points[index];

      float dist1 = np.pos[ p.GetPlane() ] - p.GetP( p.GetPlane() );

      index = index * 2; // left child
      if (dist1 > 0.0) index++; // right child

      // push the selected child to the stack
      stack[level].index = index;
      stack[level].dist2 = dist1 * dist1;
      level++;
    } // while going down the tree

    // we have arrived at a leaf
    // const CTreeNode &p = _points[index];

    // Check this leaf photon, add it if it is among the nearest neighbors
    // so far, and update the maximum square distance (np->dist2[0])
    _checkAddNearest(&np,_points[index],maxDistSqr,pred);

    int camefrom;
    do {
      camefrom = index;
      // go to parent
      index = index/2;
      --level;
      if (index == 0) {
        // Set the maximum square distance
        np.dist2[0] = maxDistSqr;
        return; // we have finished
      }
    }
    while ( (stack[level].dist2 >= np.dist2[0]) || (camefrom != stack[level].index) );
   
    // update the pointer to the tree node
    // p = _points[index];
    
    // Check this non-leaf photon, add it if ti is among the nearest
    // so far, adn update the maximum square distance (np->dist2[0])
    _checkAddNearest(&np,_points[index],maxDistSqr,pred);
    
    // step into the other subtree by changing the last bit
    index = (stack[level].index) ^ 1;    
    level++;
    
  } // while true

}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Tlist>
void 
KdTreeTmplPtr<T,TVec3>::TraverseDF(Tlist &list) const
{
  int stack[100];
  stack[0] = 1;
  int stackTop = 1;

  while(stackTop>0) {
    int index = stack[--stackTop];
    const CTreeNode &node = _points[index];

    list.push_back(node.GetPt());

    int childindex;
    if((childindex=2*index+1) <= _numPoints)
      stack[stackTop++] = childindex;  // go right
    if((childindex=2*index) <= _numPoints)
      stack[stackTop++] = childindex;  // go left
  }
}

// --------------------------------------------------------------------
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Tlist>
void 
KdTreeTmplPtr<T,TVec3>::TraverseDFInfix(Tlist &list, int index) const
{
	if(index<=_numPoints)
	{
		TraverseDFInfix(list, 2*index+1); // go right

		list.push_back(_points[index].GetPt());

		TraverseDFInfix(list, 2*index); // go left
	}
}

/*
// --------------------------------------------------------------------
//  KdTreeTmplPtr<T,TVec3>::TraverseBF()
// --------------------------------------------------------------------
template<class T, class TVec3>
template<class Tlist>
void
KdTreeTmplPtr<T,TVec3>::TraverseBF(Tlist &list) const
{
  poolqueue<int> queue(_numPoints+1);
  queue.push(1);

  while(!queue.empty()) {
    int index = queue.front();
    queue.pop();
    const CTreeNode &node = _points[index];

    list.push(node.GetPt());

    int childindex;
    if((childindex=2*index+1) <= _numPoints)
      queue.push(childindex);  // go right
    if((childindex=2*index) <= _numPoints)
      queue.push(childindex);  // go left
  }
}
*/

#endif //__KDTMPL_HXX__