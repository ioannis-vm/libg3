
Vector LovelyEig( const Matrix &M ) 
{
//.... compute eigenvalues and vectors for a 3 x 3 symmetric matrix
//
//.... INPUTS:
//        M(3,3) - matrix with initial values (only upper half used)
//
//.... OUTPUTS
//        v(3,3) - matrix of eigenvectors (by column)
//        d(3)   - eigenvalues associated with columns of v
//        rot    - number of rotations to diagonalize
//
//---------------------------------------------------------------eig3==

//.... Storage done as follows:
//
//       | v(1,1) v(1,2) v(1,3) |     |  d(1)  a(1)  a(3)  |
//       | v(2,1) v(2,2) v(2,3) |  =  |  a(1)  d(2)  a(2)  |
//       | v(3,1) v(3,2) v(3,3) |     |  a(3)  a(2)  d(3)  |
//
//        Transformations performed on d(i) and a(i) and v(i,j) become
//        the eigenvectors.  
//
//---------------------------------------------------------------eig3==

  int     rot, its, i, j , k ;
  double  g, h, aij, sm, thresh, t, c, s, tau ;

  static Matrix  v(3,3) ;
  static Vector  d(3) ;
  static Vector  a(3) ;
  static Vector  b(3) ; 
  static Vector  z(3) ;

  static const double tol = 1.0e-08 ;
 
// set v = M 
  v = M ;

//.... move array into one-d arrays

  a(0) = v(0,1) ;
  a(1) = v(1,2) ;
  a(2) = v(2,0) ;


  for ( i = 0; i < 3; i++ ) {
       d(i) = v(i,i) ;
       b(i) = v(i,i) ;
       z(i) = 0.0 ;

       for ( j = 0; j < 3; j++ ) 
  	  v(i,j) = 0.0 ;

       v(i,i) = 1.0 ;

  } //end for i

   rot = 0 ;
   its = 0 ;

   sm = fabs(a(0)) + fabs(a(1)) + fabs(a(2)) ;

   while ( sm > tol ) {
     //.... set convergence test and threshold
      if ( its < 3 ) 
        thresh = 0.011*sm ;
      else
        thresh = 0.0 ;
      
      //.... perform sweeps for rotations
      for ( i = 0; i < 3; i++ ) {

	j = (i+1)%3;
	k = (j+1)%3;

	aij  = a(i) ;

	g    = 100.0 * fabs(aij) ;

          if ( fabs(d(i)) + g != fabs(d(i))  ||
               fabs(d(j)) + g != fabs(d(j))     ) {

	      if ( fabs(aij) > thresh ) {

                a(i) = 0.0 ; 
                h    = d(j) - d(i) ; 

                if( fabs(h)+g == fabs(h) )
		   t = aij / h ;
                else {
		  //t = 2.0 * sign(h/aij) / ( fabs(h/aij) + sqrt(4.0+(h*h/aij/aij)));
		  double hDIVaij = h/aij;
		  if (hDIVaij > 0.0) 
		    t = 2.0 / (  hDIVaij + sqrt(4.0+(hDIVaij * hDIVaij)));
		  else
		    t = - 2.0 / (-hDIVaij + sqrt(4.0+(hDIVaij * hDIVaij)));
		}

//.... set rotation parameters

		 c    = 1.0 / sqrt(1.0 + t*t) ;
		 s    = t*c ;
		 tau  = s / (1.0 + c) ;

//.... rotate diagonal terms

		 h    = t * aij ;
		 z(i) = z(i) - h ;
		 z(j) = z(j) + h ;
		 d(i) = d(i) - h ;
		 d(j) = d(j) + h ;

//.... rotate off-diagonal terms

		 h    = a(j) ;
		 g    = a[k] ;
		 a(j) = h + s*(g - h*tau) ;
		 a(k) = g - s*(h + g*tau) ;

//.... rotate eigenvectors

		 for ( k = 0; k < 3; k++ ) {
		   g      = v(k,i) ;
		   h      = v(k,j) ;
		   v(k,i) = g - s*(h + g*tau) ;
		   v(k,j) = h + s*(g - h*tau) ;
		 } // end for k

		 rot = rot + 1 ;

	      } // end if fabs > thresh 
           }
           else 
             a(i) = 0.0 ;

      }  // end for i

//.... update the diagonal terms
	for ( i = 0; i < 3; i++ ) {
	   b(i) = b(i) + z(i) ;
	   d(i) = b(i) ;
	   z(i) = 0.0 ;
	} // end for i

      its += 1 ;

      sm = fabs(a(0)) + fabs(a(1)) + fabs(a(2)) ;

   } //end while sm 

   return d ;
}

