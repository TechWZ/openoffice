/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



#ifndef _BGFX_VECTOR_B2IVECTOR_HXX
#define _BGFX_VECTOR_B2IVECTOR_HXX

#include <basegfx/tuple/b2ituple.hxx>
#include <basegfx/vector/b2enums.hxx>
#include <basegfx/basegfxdllapi.h>

namespace basegfx
{
	// predeclaration
	class B2DHomMatrix;

	/** Base Point class with two sal_Int32 values

		This class derives all operators and common handling for
		a 2D data class from B2ITuple. All necessary extensions
		which are special for 2D Vectors are added here.

		@see B2ITuple
	*/
	class BASEGFX_DLLPUBLIC B2IVector : public ::basegfx::B2ITuple
	{
	public:					
		/**	Create a 2D Vector

        	The vector is initialized to (0, 0)
		*/
		B2IVector() 
		:	B2ITuple()
		{}

		/**	Create a 2D Vector

			@param nX
			This parameter is used to initialize the X-coordinate
			of the 2D Vector.
			
			@param nY
			This parameter is used to initialize the Y-coordinate
			of the 2D Vector.
		*/
		B2IVector(sal_Int32 nX, sal_Int32 nY) 
		:	B2ITuple(nX, nY)
		{}

		/**	Create a copy of a 2D Vector

			@param rVec
			The 2D Vector which will be copied.
		*/
		B2IVector(const B2IVector& rVec) 
		:	B2ITuple(rVec)
		{}

		/** constructor with tuple to allow copy-constructing
			from B2ITuple-based classes
		*/
		B2IVector(const ::basegfx::B2ITuple& rTuple) 
		:	B2ITuple(rTuple)
		{}

		~B2IVector() 
		{}

		/** *=operator to allow usage from B2IVector, too
		*/
		B2IVector& operator*=( const B2IVector& rPnt ) 
		{ 
			mnX *= rPnt.mnX;
			mnY *= rPnt.mnY; 
			return *this; 
		}

		/** *=operator to allow usage from B2IVector, too
		*/
		B2IVector& operator*=(sal_Int32 t) 
		{ 
			mnX *= t; 
			mnY *= t; 
			return *this; 
		}

		/** assignment operator to allow assigning the results
			of B2ITuple calculations
		*/
		B2IVector& operator=( const ::basegfx::B2ITuple& rVec );

		/** Calculate the length of this 2D Vector

			@return The Length of the 2D Vector
		*/
		double getLength() const;

		/** Set the length of this 2D Vector

			@param fLen
			The to be achieved length of the 2D Vector
		*/
		B2IVector& setLength(double fLen);

		/** Calculate the Scalar with another 2D Vector

			@param rVec
			The second 2D Vector

			@return
			The Scalar value of the two involved 2D Vectors
		*/
		double scalar( const B2IVector& rVec ) const;

		/** Calculate the length of the cross product with another 2D Vector

            In 2D, returning an actual vector does not make much
            sense here. The magnitude, although, can be readily
            used for tasks such as angle calculations, since for
            the returned value, the following equation holds:
            retVal = getLength(this)*getLength(rVec)*sin(theta),
            with theta being the angle between the two vectors.

			@param rVec
			The second 2D Vector

			@return
			The length of the cross product of the two involved 2D Vectors
		*/
		double cross( const B2IVector& rVec ) const;

		/** Calculate the Angle with another 2D Vector

			@param rVec
			The second 2D Vector

			@return
			The Angle value of the two involved 2D Vectors in -pi/2 < return < pi/2
		*/
		double angle( const B2IVector& rVec ) const;

		/** Transform vector by given transformation matrix.

        	Since this is a vector, translational components of the
        	matrix are disregarded.
		*/
		B2IVector& operator*=( const B2DHomMatrix& rMat );

		static const B2IVector& getEmptyVector();
	};
	
	// external operators
	//////////////////////////////////////////////////////////////////////////

	/** Calculate the orientation to another 2D Vector

		@param rVecA
		The first 2D Vector

		@param rVecB
		The second 2D Vector

		@return
		The mathematical Orientation of the two involved 2D Vectors
	*/
	BASEGFX_DLLPUBLIC B2VectorOrientation getOrientation( const B2IVector& rVecA, const B2IVector& rVecB );

	/** Calculate a perpendicular 2D Vector to the given one

		@param rVec
		The source 2D Vector

		@return
		A 2D Vector perpendicular to the one given in parameter rVec
	*/
	BASEGFX_DLLPUBLIC B2IVector getPerpendicular( const B2IVector& rVec );

	/** Test two vectors which need not to be normalized for parallelism

		@param rVecA
		The first 2D Vector

		@param rVecB
		The second 2D Vector

		@return
		bool if the two values are parallel. Also true if
		one of the vectors is empty.
	*/
	BASEGFX_DLLPUBLIC bool areParallel( const B2IVector& rVecA, const B2IVector& rVecB );

	/** Transform vector by given transformation matrix.
	    
		Since this is a vector, translational components of the
    	matrix are disregarded.
	*/
	BASEGFX_DLLPUBLIC B2IVector operator*( const B2DHomMatrix& rMat, const B2IVector& rVec );

	/** Test continuity between given vectors.
	    
		The two given vectors are assumed to describe control points on a
    	common point. Calculate if there is a continuity between them.
	*/
	BASEGFX_DLLPUBLIC B2VectorContinuity getContinuity( const B2IVector& rBackVector, const B2IVector& rForwardVector );

} // end of namespace basegfx

#endif /* _BGFX_VECTOR_B2IVECTOR_HXX */
