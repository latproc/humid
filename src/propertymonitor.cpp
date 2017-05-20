/*
	All rights reserved. Use of this source code is governed by the
	3-clause BSD License in LICENSE.txt.
*/
#include <iostream>
#include "propertymonitor.h"
#include "DragHandle.h"
#include <nanogui/imageview.h>
#include <nanogui/theme.h>
#include "lineplot.h"

using Eigen::Vector2i;

#define TO_STRING( ID ) #ID

void PositionMonitor::update(nanogui::DragHandle *dh) {
	if (dh && dh->getTarget()) {
		bool resized = false;
		switch(mMode) {
			case Handle::NONE:
				break;
			case Handle::POSITION: {
				Vector2i newPos = dh->position() + dh->size()/2 - dh->getTarget()->size()/2;
				nanogui::Widget *target_parent = dh->getTarget()->parent();
				if (target_parent && target_parent->contains(newPos + target_parent->position())
						&& target_parent->contains(newPos + dh->getTarget()->size() + target_parent->position()))
					dh->getTarget()->setPosition( newPos );

				int x = newPos.x(); 
				if (x < 0) x = 0;
				if (x > target_parent->size().x() - dh->getTarget()->size().x()) x = target_parent->size().x() - dh->getTarget()->size().x();
				int y = newPos.y(); 
				if (y < target_parent->theme()->mWindowHeaderHeight+1) y = target_parent->theme()->mWindowHeaderHeight+1;
				if (y > target_parent->size().y() - dh->getTarget()->size().y()) y = target_parent->size().y() - dh->getTarget()->size().y();
				dh->getTarget()->setPosition( Vector2i(x,y ) );

			}
				break;
			case Handle::RESIZE_TL:
				{
					nanogui::Widget *target_parent = dh->getTarget()->parent();
					Vector2i pos = dh->position() + dh->size()/2;
					Vector2i br(dh->getTarget()->position() + dh->getTarget()->size());
					Vector2i size(br - pos);
					int x = pos.x();
					if (x > target_parent->width() - dh->getTarget()->width()) 
						x = target_parent->width() - dh->getTarget()->width();
					int y = pos.y();
					if (y > target_parent->height() - dh->getTarget()->height()) 
						y = target_parent->height() - dh->getTarget()->height();
					dh->getTarget()->setPosition(Vector2i(x,y));
					if (size.x() >= 24 && size.y() >= 24) {
						resized = true;
						dh->getTarget()->setSize(size);
						nanogui::ImageView *iv = dynamic_cast<nanogui::ImageView*>(dh->getTarget());
						if (iv) iv->fit();
					}
				}
				break;
			case Handle::RESIZE_T:
				{
					nanogui::Widget *target = dh->getTarget();
					Vector2i pos(target->position().x(), dh->position().y() + dh->size().y()/2);
					Vector2i br(target->position() + target->size());
					Vector2i size(br - pos);
					dh->getTarget()->setPosition(pos);
					if (size.x() >= 24 && size.y() >= 24) {
						resized = true;
						dh->getTarget()->setSize(size);
						nanogui::ImageView *iv = dynamic_cast<nanogui::ImageView*>(dh->getTarget());
						if (iv) iv->fit();
					}
				}
				break;
			case Handle::RESIZE_TR:
				{
					nanogui::Widget *target = dh->getTarget();
					Eigen::Vector2i dh_pos(dh->position() + dh->size()/2);
					Vector2i pos(target->position().x(), dh_pos.y());
					Vector2i br(target->position() + target->size());
					Vector2i size(dh_pos.x() - target->position().x(), br.y() - dh_pos.y());
					dh->getTarget()->setPosition(pos);
					if (size.x() >= 24 && size.y() >= 24) {
						resized = true;
						dh->getTarget()->setSize(size);
						nanogui::ImageView *iv = dynamic_cast<nanogui::ImageView*>(dh->getTarget());
						if (iv) iv->fit();
					}
				}
				break;
			case Handle::RESIZE_R:
				{
					nanogui::Widget *target = dh->getTarget();
					Vector2i dh_pos(dh->position()+dh->size()/2);
					Vector2i br(target->position() + target->size()); // old br
					Vector2i size(dh_pos.x() - target->position().x(), target->size().y());
					if (size.x() >= 24 && size.y() >= 24) {
						resized = true;
						dh->getTarget()->setSize(size);
						nanogui::ImageView *iv = dynamic_cast<nanogui::ImageView*>(dh->getTarget());
						if (iv) iv->fit();
					}
				}
				break;
			case Handle::RESIZE_BL:
				{
					nanogui::Widget *target = dh->getTarget();
					Vector2i dh_pos(dh->position() + dh->size() / 2);
					Vector2i br(target->position() + target->size()); // old br
					Vector2i pos(dh_pos.x(), target->position().y());
					Vector2i size(br.x() - pos.x(), dh_pos.y() - target->position().y());
					target->setPosition(pos);
					if (size.x() >= 24 && size.y() >= 24) {
						resized = true;
						dh->getTarget()->setSize(size);
						nanogui::ImageView *iv = dynamic_cast<nanogui::ImageView*>(dh->getTarget());
						if (iv) iv->fit();
					}
				}
				break;
			case Handle::RESIZE_L:
				{
					nanogui::Widget *target = dh->getTarget();
					Vector2i dh_pos(dh->position() + dh->size() / 2);
					Vector2i pos(dh_pos.x(), target->position().y());
					Vector2i br(target->position() + target->size()); // old br
					Vector2i size(br.x() - dh_pos.x(), target->size().y());
					target->setPosition(pos);
					if (size.x() >= 24 && size.y() >= 24) {
						resized = true;
						dh->getTarget()->setSize(size);
						nanogui::ImageView *iv = dynamic_cast<nanogui::ImageView*>(dh->getTarget());
						if (iv) iv->fit();
					}
				}
				break;
			case Handle::RESIZE_BR:
				{
					Vector2i size = dh->position() + dh->size()/2 - dh->getTarget()->position();
					if (size.x() >= 24 && size.y() >= 24) {
						resized = true;
						dh->getTarget()->setSize(size);
						nanogui::ImageView *iv = dynamic_cast<nanogui::ImageView*>(dh->getTarget());
						if (iv) iv->fit();
					}
				}
				break;
			case Handle::RESIZE_B:
				{
					nanogui::Widget *target = dh->getTarget();
					Vector2i pos(dh->position()+dh->size()/2);
					Vector2i size(target->size().x(), pos.y() - target->position().y());
					if (size.x() >= 24 && size.y() >= 24) {
						resized = true;
						dh->getTarget()->setSize(size);
						nanogui::ImageView *iv = dynamic_cast<nanogui::ImageView*>(dh->getTarget());
						if (iv) iv->fit();
					}
				}
				break;
		}
		if (resized) {
			nanogui::LinePlot *lp = dynamic_cast<nanogui::LinePlot*>(dh->getTarget());
			if (lp) lp->resized();
		}
	}
}

