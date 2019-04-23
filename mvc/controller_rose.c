#include "controller_student.h"
#include "model_student.h"
#include "view_student.h"
#include <stdio.h>

static int get_roll_no(void);
static void set_roll_no(int roll_no);
static char *get_name(void);
static void set_name(char *name);
static void update(void);

controller_student c_rose = {
	.model = &m_rose,
	.view = &v_rose,
	.get_name = get_name,
	.set_name = set_name,
	.get_roll_no = get_roll_no,
	.set_roll_no = set_roll_no,
	.update = update,
};

static int get_roll_no(void)
{
	controller_student *ctrl = &c_rose;
	model_student *model = (model_student *)ctrl->model;

	return model->get_roll_no();
}

static void set_roll_no(int roll_no)
{
	controller_student *ctrl = &c_rose;
	model_student *model = (model_student *)ctrl->model;

	model->set_roll_no(roll_no);
}

static char *get_name(void)
{
	controller_student *ctrl = &c_rose;
	model_student *model = (model_student *)ctrl->model;

	return model->get_name();
}

static void set_name(char *name)
{
	controller_student *ctrl = &c_rose;
	model_student *model = (model_student *)ctrl->model;

	model->set_name(name);
}

static void update(void)
{
	controller_student *ctrl = &c_rose;
	view_student *view = (view_student *)ctrl->view;
	model_student *model = (model_student *)ctrl->model;

	view->update(model->get_name(), model->get_roll_no());
}
