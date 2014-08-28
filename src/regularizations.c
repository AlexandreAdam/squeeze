/***************************************************************/
/* Change the centroid statistic                               */
/***************************************************************/
double cent_change(int channel, double* cent_xoffset, double *cent_yoffset, long new_x, long new_y, long old_x, long old_y, unsigned short axis_len, double fov, double cent_mult)
{
    double old_sumsqr, new_sumsqr;
    old_sumsqr = (cent_xoffset[channel] * cent_xoffset[channel] + cent_yoffset[channel] * cent_yoffset[channel]);
    cent_xoffset[channel] += (double)(new_x - old_x);
    cent_yoffset[channel] += (double)(new_y - old_y);
    new_sumsqr = cent_xoffset[channel] * cent_xoffset[channel] + cent_yoffset[channel] * cent_yoffset[channel];
    return ((new_sumsqr - old_sumsqr) * cent_mult
            + ((double)(new_x - axis_len / 2) * (double)(new_x - axis_len / 2) - (double)(old_x - axis_len / 2) * (double)(old_x - axis_len / 2)
               + (double)(new_y - axis_len / 2) * (double)(new_y - axis_len / 2) - (double)(old_y - axis_len / 2) * (double)(old_y - axis_len / 2)) * fov * fov)
           * 4.0 / (double)(axis_len * axis_len);
}


/***********************/
/* The MEM functional. */
/***********************/
double entropy(unsigned long s)
{
    double ds;
    if(s < 2)
        return 0.0;
    else if(s < 8)
        {
            if(s == 2)
                return 0.69315;
            if(s == 3)
                return 1.79176;
            if(s == 4)
                return 3.17805;
            if(s == 5)
                return 4.78749;
            if(s == 6)
                return 6.57925;
            return 8.52516;
        }
    ds = (double) s;
    return ds * log(ds) - ds + 1.969;    /* From Stirling's formula */
}

/*********************************************************/
/* A function that calculates the change in dark energy. */
/* DEN_ADD is 0, DEN_SUBTRACT is 1                       */
/*********************************************************/
double den_change(double *image, unsigned short i, unsigned short j, unsigned short direction, unsigned short axis_len)
{
    double delta_den = 0.0;
    double edge_amount = 1.0;
    if(direction == DEN_INIT)
        edge_amount = 2.0;

    if((image[j * axis_len + i] == direction) || ((image[j * axis_len + i] == 0) && (direction == DEN_INIT)))
        {
            if(i == 0)
                delta_den += edge_amount;
            else if(image[axis_len * j + i - 1] == 0)
                delta_den += 1.0;
            if(j == 0)
                delta_den += edge_amount;
            else if(image[axis_len * (j - 1) + i] == 0)
                delta_den += 1.0;
            if(i == axis_len - 1)
                delta_den += edge_amount;
            else if(image[axis_len * j + i + 1] == 0)
                delta_den += 1.0;
            if(j == axis_len - 1)
                delta_den += edge_amount;
            else if(image[axis_len * (j + 1) + i] == 0)
                delta_den += 1.0;
        }
    return delta_den;
}


double transpec(int nchan, long imwidth, double *image)
{
    long i, w;
    double temp1, temp2;

    temp1 = 0;
    for(i = 0; i < imwidth * imwidth; i++)
        {
            temp2 = 0;
            for(w = 0; w < nchan; w++)
                {
                    temp2 += image[w * imwidth * imwidth + i] * image[w * imwidth * imwidth + i];
                }

            temp1 += sqrt(temp2);
        }

    return temp1;
}


double TV(const double* x, const double* pr, const double eps, const int nx, const int ny)
{
    // This regularizer is the TOTVAR (p=1.0) on the local gradient
    // the gradient is evaluated by backward difference, e is the threshold
    // by definition, L1g(x) = (sum( |grad im| ))
    // Note: all image borders are ignored

// also ignore if pixel borders an area of prior with logp < -13.8
// corresponding to a valuei n the prior image of about 1-e6

    register int i, j, off;
    double dx, dy, pixreg;
    //  double pr_threshold=-13.8;

    double L1g = - sqrt(eps) * (double)(nx * ny) ;

//pr_threshold=-1e9; // turn off.

//printf("nx ny %i %i\n",nx,ny);
    // Compute the norm of the local image gradient on each point
    for(j = 1; j < ny; j++)
        {
            off = nx * j;
            for(i = 1; i < nx; i++)
                {
                    if(i > 0)
                        {
                            dx = x[ i + off] - x[ i - 1 + off];
                            // if ( (pr[i+off] <=pr_threshold) || (pr[i-1+off] <=pr_threshold)) dx=0.0;
                        }
                    else
                        {
                            dx = x[ 1 + off] - x[ off];
                            //  if ( (pr[1+off] <=pr_threshold) || (pr[off] <=pr_threshold)) dx=0.0;
                        }

                    if(j > 0)
                        {
                            dy = x[ i + off] - x[i + off - nx];
                            // if ( (pr[i+off] <=pr_threshold) || (pr[i+off-nx] <=pr_threshold)) dy=0.0;
                        }
                    else
                        {
                            dy = x[i + nx] - x[ i ];
                            //  if ( (pr[i+nx] <=pr_threshold) || (pr[i] <=pr_threshold)) dy=0.0;
                        }


                    pixreg = sqrt(dx * dx + dy * dy + eps * eps) ;

                    L1g += pixreg;

                }
        }
    return L1g;
}

double UDreg(const double* x, const double* pr, const double eps, const int nx, const int ny)
{
    // This regularizer is the Lp norm (p=0.5) on the local gradient
    // It is somehow similar to the total variation, which is the L1 norm
    // the gradient is evaluated by backward difference, e is the threshold
    // by definition, L05g(x) = (sum( |grad im|^.5 ))^2
    // Note: all image borders are ignored

// also ignore if pixel borders an area of prior with logp < -13.8
// corresponding to a value in the prior image of about 1-e6

    register int i, j, off;
    double dx, dy, pixreg;
    //  double pr_threshold=-13.8;

    double L05g = - sqrt(eps) * (double)(nx * ny) ;

//pr_threshold=-1e9; // turn off.

//printf("nx ny %i %i\n",nx,ny);
    // Compute the norm of the local image gradient on each point
    for(j = 1; j < ny; j++)
        {
            off = nx * j;
            for(i = 1; i < nx; i++)
                {
                    if(i > 0)
                        {
                            dx = x[ i + off] - x[ i - 1 + off];
                        }
                    else
                        {
                            dx = x[ 1 + off] - x[ off];
                        }

                    if(j > 0)
                        {
                            dy = x[ i + off] - x[i + off - nx];
                        }
                    else
                        {
                            dy = x[i + nx] - x[ i ];
                        }
                    pixreg = sqrt(sqrt(dx * dx + dy * dy + eps * eps));
//  printf("i j dx dy x %i %i %5.2f %5.2f %5.2f \n",i,j,dx,dy,x[i*nx+j]);
                    L05g += pixreg;
                }
        }
//   return L05g;

    return L05g * L05g ;
}


double L0(const double* x, const double* pr, const double eps, const int nx, const int ny)
{
    register int i;
    double L0l = 0;
    for(i = 0; i < nx * ny; i++)
        {
            if(x[i] > 0)
                L0l += 1.;
        }
    return L0l;
}


double L2(const double* x, const double* pr, const double eps, const int nx, const int ny)
{
    register int i;
    double L2l = 0;
    for(i = 0; i < nx * ny; i++)
        {
            L2l += x[i] * x[i];
        }
    return L2l;
}










double LAP(const double* x, const double* pr, const double eps, const int nx, const int ny)
{
    // L1 norm of the Laplacian
    // Contrary to totvar, allows slope (good for stellar contours)
    // Laplacian via 5 point stencil  L = x[ i - 1, j] + x[i+1, j] + x[i, j-1] + x[i, j+1] - 4 * x[i,j]

    register int i, j, off;
    float L1l = 0;

    // for boundaries, we assume zeroes outside of the image
    for(j = 1; j < ny - 1; j++)
        {
            off = nx * j;
            for(i = 1; i < nx - 1; i++)
                L1l += fabs(x[ i - 1 + off] + x[i + 1 + off] + x[i + off - nx] + x[i + off + nx] - 4. * x[i + off]);
            // case i = 0
            L1l += fabs(x[1 + off] + x[off - nx] + x[off + nx] - 3. * x[off]);
            // case i = nx -1
            L1l += fabs(x[ nx - 2 + off] + x[nx - 1 + off - nx] + x[nx - 1  + off + nx] - 3. * x[nx - 1 + off]);
        }

    for(i = 1; i < nx - 1; i++)
        {
            // case j = 0
            off = 0 ;
            L1l += fabs(x[ i - 1 ] + x[i + 1] + x[i + off + nx] - 3. * x[i + off]);
            // case j = nx -1
            off = nx * (nx - 1);
            L1l += fabs(x[ i - 1 + off] + x[i + 1 + off] + x[i + off - nx] - 3. * x[i + off]);

        }

    return L1l;
}



