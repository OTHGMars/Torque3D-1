//-----------------------------------------------------------------------------
// Copyright (c) 2014 Guy Allard
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------

#ifndef _BB_COMPOSITE_H_
#define _BB_COMPOSITE_H_

#ifndef _BB_CORE_H_
#include "Core.h"
#endif

namespace BadBehavior
{
   //---------------------------------------------------------------------------
   // Composite node base class - for nodes with children
   //---------------------------------------------------------------------------
   class CompositeNode : public Node
   {
      typedef Node Parent;

   public:
      // override addObject and acceptsAsChild to only allow behavior tree nodes to be added as children
      virtual void addObject(SimObject *obj);
      virtual bool acceptsAsChild( SimObject *object ) const;
   };

   //---------------------------------------------------------------------------
   // Composite task base class
   //---------------------------------------------------------------------------
   class CompositeTask : public Task
   {
      typedef Task Parent;

   protected:
      // vector of pointers to child tasks
      VectorPtr<Task*> mChildren;

      // the current child task
      VectorPtr<Task*>::iterator mCurrentChild;

      virtual void onInitialize();
      virtual void onTerminate();
      virtual Task* update();

   public:
      CompositeTask(Node &node, SimObject &owner, BehaviorTreeRunner &runner);
      virtual ~CompositeTask();
   };
   
} // namespace BadBehavior

#endif