#include "../../src/sec.h"

pixman_image_t *pixman_image_ref (pixman_image_t *image )
{
	return NULL;
}

void pixman_transform_init_rotate ( struct pixman_transform *t,
						 	 	 	pixman_fixed_t           cos,
						 	 	 	pixman_fixed_t           sin)
{

}

pixman_bool_t pixman_transform_translate ( struct pixman_transform *forward,
						 	 	 	 	   struct pixman_transform *reverse,
						 	 	 	       pixman_fixed_t           tx,
						 	 	 	       pixman_fixed_t           ty)
{
	return 0;
}

pixman_bool_t pixman_image_set_transform (pixman_image_t           *image,
						                  const pixman_transform_t *transform)
{
	return 0;
}

pixman_bool_t pixman_image_fill_rectangles (pixman_op_t	   op,
						      	  	  	  	pixman_image_t *image,
						      	  	  	  	pixman_color_t *color,
						      	  	  	  	int			    n_rects,
						      	  	  	  	const pixman_rectangle16_t   *rects)
{
	return 0;
}

pixman_bool_t pixman_region_union ( pixman_region16_t *new_reg,
							  	    pixman_region16_t *reg1,
							  	    pixman_region16_t *reg2 )
{
	return FALSE;
}

uint32_t *pixman_image_get_data (pixman_image_t *image)
{
	return 0;
}

int pixman_image_get_width (pixman_image_t *image)
{
	return 0;
}

int pixman_image_get_height (pixman_image_t *image)
{
	return 0;
}

void pixman_f_transform_init_identity (struct pixman_f_transform *t)
{

}

pixman_bool_t pixman_f_transform_scale (struct pixman_f_transform *forward,
										struct pixman_f_transform *reverse,
										double                    sx,
										double                    sy)
{
	return FALSE;
}

pixman_bool_t pixman_f_transform_translate (struct pixman_f_transform *forward,
											struct pixman_f_transform *reverse,
											double                    tx,
											double                    ty)
{
	return FALSE;
}

pixman_bool_t pixman_f_transform_rotate (struct pixman_f_transform *forward,
										 struct pixman_f_transform *reverse,
										 double                    c,
										 double                    s)
{
	return FALSE;
}

pixman_bool_t pixman_transform_from_pixman_f_transform (struct pixman_transform *t,
														const struct pixman_f_transform *ft)
{
	return FALSE;
}








































