#include <iostream>
#include <vector>
#include <cmath>

#ifndef M_PI
#define M_PI (3.14159265358979323846)
#endif

// https://ccrma.stanford.edu/~jos/resample/resample.pdf
// http://www.eecs.umich.edu/courses/eecs206/archive/spring02/notes.dir/interpol.pdf


void show( std::vector<double> data, int begin=0, int end=0  ) {
	if ( 0 == end )
		end = data.size();

	std::cout << "\t\t +-";
	for ( int jjj = 0; jjj < 80; ++jjj )
		if ( 15 == jjj || 40 == jjj || 65 == jjj ) 
			std::cout << "+";
		else
			std::cout << "-";
	std::cout << ">\n";
	auto dataIt = data.cbegin() + begin;
	for ( int iii=begin; iii < end; ++iii, ++dataIt ) {
		int value = 100 * *dataIt;
		std::cout.precision(3);
		std::cout << iii << "\t:" << std::fixed << *dataIt << "\t | ";
		value = 40 + ( value + 2) / 4;
		for ( int jjj = 0; jjj < 80; ++jjj )
			if ( jjj == value ) 
				std::cout << "*";
			else if ( 40 == jjj ) 
				std::cout << ".";
			else
				std::cout << " ";
			std::cout << "\n";
	}

	std::cout << "\t\t +-";
	for ( int jjj = 0; jjj < 80; ++jjj )
		if ( 15 == jjj || 40 == jjj || 65 == jjj ) 
			std::cout << "+";
		else
			std::cout << "-";
	std::cout << ">\n";
}

	
static const int oversample = 5;
static const int sinc_width = 4;

static const int sincSize = sinc_width * oversample;

static std::vector <double> sinc;


void prepareSinc() {
	std::cout << "prepareSinc( " << sincSize << " )\n";
	sinc.clear();
	sinc.resize( sincSize );
	auto sincIt = sinc.begin();
	for ( int pos = 1; pos <= sincSize; ++pos, ++sincIt ) {
		double t = pos * M_PI / oversample;
		// Hann window: 0.5 + 0.5 cos, Hamming: 0.54 + 0.46 cos
		double w = 0.54 + 0.46 * cos( pos * M_PI / sincSize );
		// w = 1;
		*sincIt = w * sin( t ) / t;
	}
}


int main() {

	// here we come with previously calculated values
	const int skipTrigger = 123; // example value, changes from trace to trace
	const int dotsOnScreen = 31; // depends on sample rate and time/div
	
	int left = skipTrigger;
	int right = skipTrigger + dotsOnScreen;
	// we have this layout: | skipTrigger | dotsOnScreen | sincSize | ... a lot more data ... |
	int sampleSize = skipTrigger + dotsOnScreen + sincSize;

	std::cout << "acquire data\n";
	// simulate a cut-out of the big 20k sample buffer
	// with left and right enough margin for the sinc window
	// left depends on skipTtrigger, in worst case it's only 2..5
	std::vector <double> samples;
	samples.resize( sampleSize );
	for ( int pos = 0; pos < left+9; ++pos )
		samples[ pos ] = 1.0;
	
	//for ( int pos = left+9; pos < left+15; ++pos )
	//	samples[ pos ] = sin( 2 * pos );
	samples[ left + 14 ] = 1.0;
	samples[ left + 16 ] = - 1.0;
	for ( int pos = right-9; pos < sampleSize; ++pos )
		samples[ pos ] = -1.0;
	show( samples, left, right );


	prepareSinc();
	show( sinc );


	// here we come with sampled data in "samples"
	// display should start at pos "skipTrigger" 
	
	std::cout << "convolute samples * sinc\n";

	left = std::min( sincSize, skipTrigger ); // we would need sincSize, but we take what we get
	right = left + dotsOnScreen;
	sampleSize = left + dotsOnScreen + sincSize;

	// create a display buffer with additional left and right margin
	std::vector <double> result;
	unsigned resultSize = sampleSize * oversample;
	result.resize( resultSize );

	// sampleIt points to start of left margin
	auto sampleIt = samples.cbegin() + skipTrigger - left;
	for ( int resultPos = 0; resultPos < resultSize; resultPos += oversample, ++sampleIt ) {
		result[ resultPos ] += *sampleIt; //  * sinc( 0 )
		auto sincIt = sinc.cbegin();
		for ( int sincPos = 1; sincPos <= sincSize; ++sincPos, ++sincIt ) { // sinc( 1..n )
			double conv = *sampleIt * *sincIt;
			int pos = resultPos - sincPos; // left half of sinc
			if ( pos >= 0 ) 
				result[ pos ] += conv;
			pos = resultPos + sincPos; // right half of sinc
			if ( pos < resultSize )
				result[ pos ] += conv;
		}
	}


	std::cout << "display result\n";

	// display the upsampled values in the original time range.
    show( result, left*oversample, right*oversample );


}
