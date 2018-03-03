/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/

#ifndef itkDescoteauxEigenToMeasureParameterEstimationFilter_hxx
#define itkDescoteauxEigenToMeasureParameterEstimationFilter_hxx

#include "itkDescoteauxEigenToMeasureParameterEstimationFilter.h"

namespace itk {

template< typename TInputImage, typename TInputSpatialObject >
DescoteauxEigenToMeasureParameterEstimationFilter< TInputImage, TInputSpatialObject >
::DescoteauxEigenToMeasureParameterEstimationFilter() :
  Superclass(),
  m_FrobeniusNormWeight(0.5)
{
  /* Set parameter size to 3 */
  ParameterArrayType parameters = this->GetParametersOutput()->Get();
  parameters.SetSize(3);
  parameters[0] = 0.5;
  parameters[1] = 0.5;
  parameters[2] = 1;
  this->GetParametersOutput()->Set(parameters);
}

template< typename TInputImage, typename TInputSpatialObject >
void
DescoteauxEigenToMeasureParameterEstimationFilter< TInputImage, TInputSpatialObject >
::BeforeThreadedGenerateData()
{
  ThreadIdType numberOfCalls = this->GetNumberOfThreads()*this->GetNumberOfStreamDivisions();

  /* Resize threads */
  m_MaxFrobeniusNorm.SetSize(numberOfCalls);
  m_MaxFrobeniusNorm.Fill(NumericTraits< RealType >::ZeroValue());
}

template< typename TInputImage, typename TInputSpatialObject >
void
DescoteauxEigenToMeasureParameterEstimationFilter< TInputImage, TInputSpatialObject >
::AfterThreadedGenerateData()
{
  ThreadIdType numberOfCalls = this->GetNumberOfThreads()*this->GetNumberOfStreamDivisions();

  /* Determine default parameters */
  RealType alpha, beta, c;
  alpha = 0.5f;
  beta = 0.5f;
  c = 0.0f;

  /* Accumulate over threads */
  RealType maxFrobeniusNorm = NumericTraits< RealType >::ZeroValue();

  for (unsigned int i = 0; i < numberOfCalls; ++i )
  {
    if (m_MaxFrobeniusNorm[i] > maxFrobeniusNorm)
    {
      maxFrobeniusNorm = m_MaxFrobeniusNorm[i];
    }
  }

  /* Scale c */
  if (maxFrobeniusNorm > 0) {
    c = m_FrobeniusNormWeight * maxFrobeniusNorm;
  }

  /* Assign outputs parameters */
  ParameterArrayType parameters;
  parameters.SetSize(3);
  parameters[0] = alpha;
  parameters[1] = beta;
  parameters[2] = c;
  this->GetParametersOutput()->Set(parameters);
}

template< typename TInputImage, typename TInputSpatialObject >
void
DescoteauxEigenToMeasureParameterEstimationFilter< TInputImage, TInputSpatialObject >
::ThreadedGenerateData(const OutputImageRegionType & outputRegionForThread, ThreadIdType threadId)
{
  /* If size is zero, return */
  const SizeValueType size0 = outputRegionForThread.GetSize(0);
  if (size0 == 0)
  {
    return;
  }

  /* Count starts zero */
  RealType maxFrobeniusNorm = NumericTraits< RealType >::ZeroValue();
  RealType thisFrobeniusNorm;

  /* Get input and mask pointer */
  InputImageConstPointer inputPointer = this->GetInput();
  SpatialObjectConstPointer maskPointer = this->GetMaskingSpatialObject();
  typename InputImageType::PointType point;

  OutputImageType      *outputPtr = this->GetOutput(0);
  
  // Define the portion of the input to walk for this thread, using
  // the CallCopyOutputRegionToInputRegion method allows for the input
  // and output images to be different dimensions
  InputImageRegionType inputRegionForThread;

  this->CallCopyOutputRegionToInputRegion(inputRegionForThread, outputRegionForThread);

  /* Setup iterator */
  ImageRegionConstIteratorWithIndex< TInputImage > inputIt(inputPointer, inputRegionForThread);
  ImageRegionIterator< OutputImageType > outputIt(outputPtr, outputRegionForThread);

  /* Iterate and count */
  inputIt.GoToBegin();
  outputIt.GoToBegin();
  while ( !inputIt.IsAtEnd() )
  {
    // Process point
    inputPointer->TransformIndexToPhysicalPoint(inputIt.GetIndex(), point);
    if ( (!maskPointer) ||  (maskPointer->IsInside(point)) )
    {
      /* Compute max norm */
      thisFrobeniusNorm = this->CalculateFrobeniusNorm(inputIt.Get());
      if (thisFrobeniusNorm > maxFrobeniusNorm)
      {
        maxFrobeniusNorm = thisFrobeniusNorm;
      }
    }

    // Set 
    outputIt.Set( static_cast< OutputImagePixelType >( inputIt.Get() ) );

    // Increment
    ++inputIt;
    ++outputIt;
  }

  /* Store this thread */
  m_MaxFrobeniusNorm[threadId] = maxFrobeniusNorm;
}

template< typename TInputImage, typename TInputSpatialObject >
typename DescoteauxEigenToMeasureParameterEstimationFilter< TInputImage, TInputSpatialObject >::RealType
DescoteauxEigenToMeasureParameterEstimationFilter< TInputImage, TInputSpatialObject >
::CalculateFrobeniusNorm(const InputImagePixelType& pixel) const
{
  /* Forbenius norm is given by the square root of the sum of squares 
  * of the eigenvalues for real, symmetric matricies
  */
  RealType norm = 0;
  for( unsigned int i = 0; i < pixel.Length; ++i) {
    norm += pixel[i]*pixel[i];
  }
  return sqrt(norm);
}

template< typename TInputImage, typename TInputSpatialObject >
void
DescoteauxEigenToMeasureParameterEstimationFilter< TInputImage, TInputSpatialObject >
::PrintSelf(std::ostream & os, Indent indent) const
{
  Superclass::PrintSelf(os, indent);
  os << indent << "FrobeniusNormWeight: " << GetFrobeniusNormWeight() << std::endl;
}

} /* end namespace */

#endif /* itkDescoteauxEigenToMeasureParameterEstimationFilter_hxx */